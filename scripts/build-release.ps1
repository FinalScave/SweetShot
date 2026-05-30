param(
    [Alias("p")]
    [ValidateSet("all", "windows", "linux", "linux-x86_64", "linux-aarch64")]
    [string]$Platform = "windows",

    [Alias("b")]
    [string]$Build = "",

    [Alias("s")]
    [string]$Src = "",

    [string]$SkillDir = "",
    [string]$WslDistro = ""
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = if ($Src) {
    [System.IO.Path]::GetFullPath($Src)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $ScriptDir ".."))
}
$BuildDir = if ($Build) {
    [System.IO.Path]::GetFullPath($Build)
} else {
    Join-Path $ProjectDir "build\release"
}
$ResolvedSkillDir = if ($SkillDir) {
    [System.IO.Path]::GetFullPath($SkillDir)
} else {
    Join-Path $ProjectDir "skill\sweetshot-code-screenshot"
}

function Write-Section {
    param([Parameter(Mandatory = $true)][string]$Title)
    Write-Host ""
    Write-Host "============================= $Title ============================="
}

function Invoke-External {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        $joined = ($Arguments | ForEach-Object {
            if ($_ -match '\s') { '"{0}"' -f $_ } else { $_ }
        }) -join ' '
        throw "Command failed: $FilePath $joined"
    }
}

function Ensure-Directory {
    param([Parameter(Mandatory = $true)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

function Resolve-CommandPath {
    param([Parameter(Mandatory = $true)][string]$Name)
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) {
        throw "Required command not found: $Name"
    }
    return $command.Source
}

function Get-WslDistros {
    param([Parameter(Mandatory = $true)][string]$WslPath)

    $rawLines = & $WslPath -l -q
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to list installed WSL distributions."
    }

    $distros = @()
    foreach ($line in $rawLines) {
        $clean = ($line -replace "`0", "").Trim()
        if ($clean) {
            $distros += $clean
        }
    }
    return $distros
}

function Resolve-WslDistroName {
    param(
        [Parameter(Mandatory = $true)][string]$WslPath,
        [string]$RequestedName = ""
    )

    $distros = @(Get-WslDistros -WslPath $WslPath)
    if ($distros.Count -eq 0) {
        throw "No WSL distributions are installed."
    }

    if ($RequestedName) {
        $exactMatches = @($distros | Where-Object { $_ -ieq $RequestedName })
        if ($exactMatches.Count -eq 1) {
            return $exactMatches[0]
        }

        $prefixMatches = @($distros | Where-Object {
            $_.StartsWith($RequestedName, [System.StringComparison]::OrdinalIgnoreCase)
        })
        if ($prefixMatches.Count -eq 1) {
            return $prefixMatches[0]
        }

        $installed = $distros -join ", "
        throw "WSL distribution '$RequestedName' was not found or is ambiguous. Installed distros: $installed"
    }

    $preferred = @($distros | Where-Object { $_ -notmatch '^docker-desktop(?:-data)?$' })
    if ($preferred.Count -eq 0) {
        $installed = $distros -join ", "
        throw "No usable WSL build distribution found. Installed distros: $installed"
    }
    return $preferred[0]
}

function Convert-ToWslPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($fullPath -match '^([A-Za-z]):\\?(.*)$') {
        $drive = $matches[1].ToLowerInvariant()
        $rest = $matches[2] -replace '\\', '/'
        if ([string]::IsNullOrEmpty($rest)) {
            return "/mnt/$drive"
        }
        return "/mnt/$drive/$rest"
    }
    throw "Unsupported Windows path for WSL conversion: $fullPath"
}

function Convert-ToBashSingleQuoted {
    param([Parameter(Mandatory = $true)][string]$Value)
    return "'" + ($Value -replace "'", "'\''") + "'"
}

function Find-SweetShotExe {
    param([Parameter(Mandatory = $true)][string]$BuildPath)

    $candidates = @(
        (Join-Path $BuildPath "bin\Release\sweetshot.exe"),
        (Join-Path $BuildPath "bin\sweetshot.exe"),
        (Join-Path $BuildPath "Release\sweetshot.exe"),
        (Join-Path $BuildPath "sweetshot.exe")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }
    throw "Built sweetshot.exe was not found under $BuildPath"
}

function Build-Windows {
    Write-Section "Windows"

    $windowsBuildDir = Join-Path $BuildDir "windows"
    $windowsBinDir = Join-Path $ResolvedSkillDir "bin\windows"
    Ensure-Directory -Path $windowsBinDir

    Invoke-External -FilePath "cmake" -Arguments @(
        "-S", $ProjectDir,
        "-B", $windowsBuildDir,
        "-G", "Visual Studio 17 2022",
        "-DSWEETSHOT_BUILD_CLI=ON",
        "-DSWEETSHOT_BUILD_TESTS=OFF",
        "-DSWEETSHOT_PNG_BACKEND=resvg"
    )

    Invoke-External -FilePath "cmake" -Arguments @(
        "--build", $windowsBuildDir,
        "--target", "sweetshot",
        "--config", "Release",
        "--parallel"
    )

    $sourceExe = Find-SweetShotExe -BuildPath $windowsBuildDir
    $targetExe = Join-Path $windowsBinDir "sweetshot.exe"
    Copy-Item -LiteralPath $sourceExe -Destination $targetExe -Force

    $oldDll = Join-Path $windowsBinDir "resvg.dll"
    if (Test-Path -LiteralPath $oldDll) {
        Remove-Item -LiteralPath $oldDll -Force
    }

    Write-Host "Updated $targetExe"
}

function Invoke-WslRelease {
    param([Parameter(Mandatory = $true)][string]$LinuxPlatform)

    Write-Section "WSL $LinuxPlatform"
    $wsl = Resolve-CommandPath -Name "wsl.exe"
    $distro = Resolve-WslDistroName -WslPath $wsl -RequestedName $WslDistro
    Write-Host "Distro: $distro"

    $scriptWsl = Convert-ToWslPath -Path (Join-Path $ScriptDir "build-release.sh")
    $projectWsl = Convert-ToWslPath -Path $ProjectDir
    $buildWsl = Convert-ToWslPath -Path $BuildDir
    $skillWsl = Convert-ToWslPath -Path $ResolvedSkillDir

    $bashCommand = @(
        "bash",
        (Convert-ToBashSingleQuoted -Value $scriptWsl),
        "-p", (Convert-ToBashSingleQuoted -Value $LinuxPlatform),
        "-s", (Convert-ToBashSingleQuoted -Value $projectWsl),
        "-b", (Convert-ToBashSingleQuoted -Value $buildWsl),
        "--skill-dir", (Convert-ToBashSingleQuoted -Value $skillWsl)
    ) -join " "

    Invoke-External -FilePath $wsl -Arguments @("-d", $distro, "--", "bash", "-lc", $bashCommand)
}

Ensure-Directory -Path $BuildDir
Ensure-Directory -Path $ResolvedSkillDir

switch ($Platform) {
    "all" {
        Build-Windows
        Invoke-WslRelease -LinuxPlatform "linux"
    }
    "windows" {
        Build-Windows
    }
    "linux" {
        Invoke-WslRelease -LinuxPlatform "linux"
    }
    "linux-x86_64" {
        Invoke-WslRelease -LinuxPlatform "linux-x86_64"
    }
    "linux-aarch64" {
        Invoke-WslRelease -LinuxPlatform "linux-aarch64"
    }
}
