Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$ffplayPath = Join-Path $PSScriptRoot "..\Binary\ffplay.exe"
if (-not (Test-Path $ffplayPath)) {
    [System.Windows.Forms.MessageBox]::Show("ffplay.exe not found at $ffplayPath", "Missing binary", "OK", "Error") | Out-Null
    exit 1
}

[System.Windows.Forms.Application]::EnableVisualStyles()

$form = New-Object System.Windows.Forms.Form
$form.Text = "STM32 UVC Viewer"
$form.StartPosition = "CenterScreen"
$form.ClientSize = New-Object System.Drawing.Size(360, 150)
$form.FormBorderStyle = "FixedDialog"
$form.MaximizeBox = $false

$deviceLabel = New-Object System.Windows.Forms.Label
$deviceLabel.Text = "Device name"
$deviceLabel.Location = New-Object System.Drawing.Point(10, 15)
$deviceLabel.AutoSize = $true
$form.Controls.Add($deviceLabel)

$deviceInput = New-Object System.Windows.Forms.TextBox
$deviceInput.Text = "STM32 uvc"
$deviceInput.Location = New-Object System.Drawing.Point(100, 12)
$deviceInput.Width = 240
$form.Controls.Add($deviceInput)

$statusLabel = New-Object System.Windows.Forms.Label
$statusLabel.Text = "Status: idle"
$statusLabel.Location = New-Object System.Drawing.Point(10, 110)
$statusLabel.AutoSize = $true
$form.Controls.Add($statusLabel)

$startButton = New-Object System.Windows.Forms.Button
$startButton.Text = "Start"
$startButton.Location = New-Object System.Drawing.Point(100, 55)
$startButton.Width = 80
$form.Controls.Add($startButton)

$stopButton = New-Object System.Windows.Forms.Button
$stopButton.Text = "Stop"
$stopButton.Location = New-Object System.Drawing.Point(200, 55)
$stopButton.Width = 80
$form.Controls.Add($stopButton)

$ffplayProcess = $null

function Stop-Player {
    if ($null -eq $ffplayProcess -or $ffplayProcess.HasExited) {
        $statusLabel.Text = "Status: idle"
        return
    }

    $statusLabel.Text = "Status: stopping..."
    try {
        $ffplayProcess.CloseMainWindow() | Out-Null
        if (-not $ffplayProcess.WaitForExit(1500)) {
            $ffplayProcess.Kill()
            $ffplayProcess.WaitForExit()
        }
        $statusLabel.Text = "Status: stopped"
    } catch {
        $statusLabel.Text = "Status: stop failed: $($_.Exception.Message)"
    }
}

$startButton.Add_Click({
    if ($ffplayProcess -and -not $ffplayProcess.HasExited) {
        $statusLabel.Text = "Status: already running"
        return
    }

    $deviceName = $deviceInput.Text.Trim()
    if ([string]::IsNullOrWhiteSpace($deviceName)) {
        $statusLabel.Text = "Status: enter a device name"
        return
    }

    $statusLabel.Text = "Status: launching..."
    $deviceArg = "video=`"$deviceName`""

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $ffplayPath
    $psi.Arguments = "-f dshow -i $deviceArg"
    $psi.WorkingDirectory = Split-Path $ffplayPath
    $psi.UseShellExecute = $false

    try {
        $ffplayProcess = [System.Diagnostics.Process]::Start($psi)
        $statusLabel.Text = "Status: running"
    } catch {
        $statusLabel.Text = "Status: launch failed"
        [System.Windows.Forms.MessageBox]::Show("Failed to launch ffplay: $($_.Exception.Message)", "Launch error", "OK", "Error") | Out-Null
    }
})

$stopButton.Add_Click({ Stop-Player })
$form.Add_FormClosing({ Stop-Player })

[System.Windows.Forms.Application]::Run($form)
