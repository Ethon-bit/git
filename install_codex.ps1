$ErrorActionPreference = "Stop"
$DestDir = "D:\codex"

Write-Host "=== Codex CLI === D盘安装 ==="

if (-not (Test-Path $DestDir)) {
    New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
}

Write-Host "[1/3] 获取最新版本..."
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/openai/codex/releases/latest" -Headers @{"User-Agent"="curl"}
Write-Host "  版本: $($release.tag_name)"

# Find codex CLI x86_64 windows exe.zip (not app-server, not symbols, not command-runner, not sandbox-setup)
$targetPattern = "codex-x86_64-pc-windows-msvc.exe.zip"
$codexAsset = $null
$assets = $release.assets
for ($i = 0; $i -lt $assets.Count; $i++) {
    if ($assets[$i].name -eq $targetPattern) {
        $codexAsset = $assets[$i]
        break
    }
}

# Fallback: direct exe
if (-not $codexAsset) {
    $targetPattern = "codex-x86_64-pc-windows-msvc.exe"
    for ($i = 0; $i -lt $assets.Count; $i++) {
        if ($assets[$i].name -eq $targetPattern) {
            $codexAsset = $assets[$i]
            break
        }
    }
}

if (-not $codexAsset) {
    Write-Host "[FAIL] 未找到 codex-x86_64-pc-windows-msvc.exe"
    exit 1
}

$sizeMB = [math]::Round($codexAsset.size / 1MB, 2)
Write-Host "[2/3] 下载 $($codexAsset.name) ($sizeMB MB)..."
$dlPath = "$DestDir\$($codexAsset.name)"
Invoke-WebRequest -Uri $codexAsset.browser_download_url -OutFile $dlPath

# If zip, extract
if ($dlPath.EndsWith(".zip")) {
    Write-Host "[3/3] 解压..."
    Expand-Archive -Path $dlPath -DestinationPath $DestDir -Force
    Remove-Item $dlPath
}

Write-Host ""
Write-Host "解压后文件:"
Get-ChildItem -Path $DestDir -File | ForEach-Object {
    $s = [math]::Round($_.Length / 1MB, 2)
    Write-Host "  $($_.Name) ($s MB)"
}

# Also download sandbox setup
Write-Host ""
Write-Host "[额外] 下载沙箱组件..."
$sandboxName = "codex-windows-sandbox-setup-x86_64-pc-windows-msvc.exe"
$sandboxAsset = $null
for ($i = 0; $i -lt $assets.Count; $i++) {
    if ($assets[$i].name -eq $sandboxName) {
        $sandboxAsset = $assets[$i]
        break
    }
}
if ($sandboxAsset) {
    Invoke-WebRequest -Uri $sandboxAsset.browser_download_url -OutFile "$DestDir\$sandboxName"
    Write-Host "  $sandboxName - 下载完成"
    Write-Host "  (需要以管理员身份运行此文件以配置沙箱)"
}

Write-Host ""
Write-Host "========================================"
Write-Host "  Codex CLI 安装完成!"
Write-Host "  路径: $DestDir"
Write-Host "========================================"
Write-Host ""
Write-Host "下一步:"
Write-Host "  1. 将 D:\codex 添加到系统 PATH 环境变量"
Write-Host "  2. (管理员) 运行 D:\codex\$sandboxName 配置沙箱"
Write-Host "  3. 打开新终端，运行: codex"
