using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

namespace FfplayLauncher
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new MainForm());
        }
    }

    public class MainForm : Form
    {
        private readonly TextBox _deviceInput;
        private readonly Label _statusLabel;
        private readonly Button _startButton;
        private readonly Button _stopButton;
        private Process _ffplayProcess;
        private readonly string _ffplayPath;

        public MainForm()
        {
            Text = "STM32 UVC Viewer";
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            ClientSize = new System.Drawing.Size(360, 150);

            var deviceLabel = new Label
            {
                Text = "Device name",
                AutoSize = true,
                Left = 10,
                Top = 15
            };
            Controls.Add(deviceLabel);

            _deviceInput = new TextBox
            {
                Text = "STM32 uvc",
                Left = 100,
                Top = 12,
                Width = 240
            };
            Controls.Add(_deviceInput);

            _startButton = new Button
            {
                Text = "Start",
                Left = 100,
                Top = 55,
                Width = 80
            };
            _startButton.Click += StartClicked;
            Controls.Add(_startButton);

            _stopButton = new Button
            {
                Text = "Stop",
                Left = 200,
                Top = 55,
                Width = 80
            };
            _stopButton.Click += StopClicked;
            Controls.Add(_stopButton);

            _statusLabel = new Label
            {
                Text = "Status: idle",
                AutoSize = true,
                Left = 10,
                Top = 110
            };
            Controls.Add(_statusLabel);

            _ffplayPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "ffplay.exe");
            if (!File.Exists(_ffplayPath))
            {
                MessageBox.Show(this, "ffplay.exe not found next to this app.", "Missing ffplay", MessageBoxButtons.OK, MessageBoxIcon.Error);
                _startButton.Enabled = false;
            }

            FormClosing += (s, e) => StopPlayer();
        }

        private void StartClicked(object sender, EventArgs e)
        {
            if (_ffplayProcess != null && !_ffplayProcess.HasExited)
            {
                _statusLabel.Text = "Status: already running";
                return;
            }

            var deviceName = _deviceInput.Text.Trim();
            if (string.IsNullOrEmpty(deviceName))
            {
                _statusLabel.Text = "Status: enter a device name";
                return;
            }

            if (!File.Exists(_ffplayPath))
            {
                _statusLabel.Text = "Status: ffplay missing";
                return;
            }

            var psi = new ProcessStartInfo
            {
                FileName = _ffplayPath,
                Arguments = "-f dshow -i video=\"" + deviceName + "\"",
                WorkingDirectory = Path.GetDirectoryName(_ffplayPath),
                UseShellExecute = false
            };

            try
            {
                _ffplayProcess = Process.Start(psi);
                _statusLabel.Text = "Status: running";
            }
            catch (Exception ex)
            {
                _statusLabel.Text = "Status: launch failed";
                MessageBox.Show(this, "Failed to launch ffplay:\n" + ex.Message, "Launch error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void StopClicked(object sender, EventArgs e)
        {
            StopPlayer();
        }

        private void StopPlayer()
        {
            if (_ffplayProcess == null || _ffplayProcess.HasExited)
            {
                _statusLabel.Text = "Status: idle";
                return;
            }

            _statusLabel.Text = "Status: stopping...";
            try
            {
                _ffplayProcess.CloseMainWindow();
                if (!_ffplayProcess.WaitForExit(1500))
                {
                    _ffplayProcess.Kill();
                    _ffplayProcess.WaitForExit();
                }
                _statusLabel.Text = "Status: stopped";
            }
            catch (Exception ex)
            {
                _statusLabel.Text = "Status: stop failed";
                MessageBox.Show(this, "Failed to stop ffplay:\n" + ex.Message, "Stop error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
    }
}
