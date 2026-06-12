using System.IO.Ports;
using EmsApp.Core;

namespace EmsApp.Serial;

// Wraps a SerialPort connection to one CanbusMonitor.
// Call Open() before use.  Dispose() closes the port.
// Received CAN frames are raised via FrameReceived.
// Call SendCommand() to send any DeviceCommand string.
public sealed class DeviceConnection : IDisposable
{
    private readonly SerialPort _port;
    private bool _disposed;

    public event Action<CanFrame>? FrameReceived;
    public event Action<string>?   RawLineReceived;

    public DeviceConnection(string portName, int baudRate = 115200)
    {
        _port = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One)
        {
            NewLine          = "\n",
            ReadTimeout      = 2000,
            WriteTimeout     = 2000,
            DtrEnable        = true,
        };
    }

    public bool IsOpen => _port.IsOpen;

    public void Open()
    {
        _port.Open();
        _port.DataReceived += OnData;
    }

    public void SendCommand(string command)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        _port.WriteLine(command);
    }

    private void OnData(object sender, SerialDataReceivedEventArgs e)
    {
        try
        {
            while (_port.BytesToRead > 0)
            {
                string line = _port.ReadLine().Trim();
                RawLineReceived?.Invoke(line);

                CanFrame? frame = TryParseFrame(line);
                if (frame is not null)
                    FrameReceived?.Invoke(frame);
            }
        }
        catch (TimeoutException) { }
    }

    // Parses "CAN:0x<id>:<hexbytes>" into a CanFrame.
    private static CanFrame? TryParseFrame(string line)
    {
        if (!line.StartsWith("CAN:0x", StringComparison.Ordinal)) return null;

        int colonPos = line.IndexOf(':', 6);
        if (colonPos < 0) return null;

        if (!uint.TryParse(line[6..colonPos], System.Globalization.NumberStyles.HexNumber, null, out uint canId))
            return null;

        string hexBytes = line[(colonPos + 1)..];
        if (hexBytes.Length % 2 != 0) return null;

        byte[] data = new byte[hexBytes.Length / 2];
        for (int i = 0; i < data.Length; i++)
            data[i] = Convert.ToByte(hexBytes.Substring(i * 2, 2), 16);

        return new CanFrame(canId, (byte)data.Length, data, DateTimeOffset.UtcNow.ToString("o"), 0);
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _port.DataReceived -= OnData;
        if (_port.IsOpen) _port.Close();
        _port.Dispose();
    }
}
