$WshShell = New-Object -ComObject WScript.Shell
$DesktopPath = [Environment]::GetFolderPath("Desktop")

# Remove old shortcuts
$old1 = Join-Path $DesktopPath "Codex.lnk"
$old2 = Join-Path $DesktopPath "Codex App.lnk"
$old3 = Join-Path $DesktopPath "Codex CLI.lnk"
Remove-Item $old1 -Force -ErrorAction SilentlyContinue
Remove-Item $old2 -Force -ErrorAction SilentlyContinue
Remove-Item $old3 -Force -ErrorAction SilentlyContinue

# Codex App shortcut
$s1 = $WshShell.CreateShortcut((Join-Path $DesktopPath "Codex App.lnk"))
$s1.TargetPath = "D:\codex\codex.exe"
$s1.Arguments = "app"
$s1.WorkingDirectory = "D:\codex"
$s1.Description = "Codex Desktop App"
$s1.Save()

# Codex CLI shortcut
$s2 = $WshShell.CreateShortcut((Join-Path $DesktopPath "Codex CLI.lnk"))
$s2.TargetPath = "D:\codex\codex.bat"
$s2.WorkingDirectory = "D:\codex"
$s2.Description = "Codex Command Line"
$s2.Save()

Write-Host "Shortcuts created: Codex App.lnk and Codex CLI.lnk"
