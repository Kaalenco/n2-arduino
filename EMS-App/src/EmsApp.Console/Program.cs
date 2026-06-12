using EmsApp.Core;
using EmsApp.Serial;

// ems <port> [command [args...]]
//
// Without a command: enters live monitor mode — prints every CAN frame to stdout.
// With a command: sends the command and exits.
//
// Commands:
//   time                      Set RTC to current UTC time
//   devid <hex4>              Set device ID
//   aircraft <hex4>           Set aircraft ID
//   speed <125|250|500>       Set CAN bus speed (kbps)
//   clear                     Clear message store
//   list                      Dump tracked IDs
//   reset                     Software-reset the device
//   download <yyyyMMdd>       Download log file and print to stdout
//   sysreset                  Broadcast SYSRESET to all CAN nodes
//   set <TYPE> <PARAM> <val>  Send CONFIG to instrument (e.g. set RPM GRL 700)
//                             Param mnemonics: see docs/can-param-mnemonics.md

if (args.Length < 1)
{
    Console.Error.WriteLine("Usage: ems <port> [command [args...]]");
    return 1;
}

string port = args[0];
using var device = new DeviceConnection(port);

try
{
    device.Open();
}
catch (Exception ex)
{
    Console.Error.WriteLine($"Failed to open {port}: {ex.Message}");
    return 2;
}

if (args.Length == 1)
{
    // Live monitor
    Console.Error.WriteLine($"Connected to {port}. Press Ctrl+C to exit.");
    device.RawLineReceived += line => Console.WriteLine(line);
    using var cts = new CancellationTokenSource();
    Console.CancelKeyPress += (_, e) => { e.Cancel = true; cts.Cancel(); };
    await Task.Delay(Timeout.Infinite, cts.Token).ContinueWith(_ => { });
    return 0;
}

string cmd = args[1].ToLowerInvariant();
string? response = cmd switch
{
    "time"      => DeviceCommand.SetTime(DateTimeOffset.UtcNow),
    "clear"     => DeviceCommand.ClearStore(),
    "list"      => DeviceCommand.ListStore(),
    "reset"     => DeviceCommand.Reset(),
    "devid"     => args.Length > 2 && ushort.TryParse(args[2], System.Globalization.NumberStyles.HexNumber, null, out ushort did)
                       ? DeviceCommand.SetDeviceId(did) : null,
    "aircraft"  => args.Length > 2 && ushort.TryParse(args[2], System.Globalization.NumberStyles.HexNumber, null, out ushort aid)
                       ? DeviceCommand.SetAircraftId(aid) : null,
    "speed"     => args.Length > 2 && int.TryParse(args[2], out int spd)
                       ? DeviceCommand.SetCanSpeed(spd) : null,
    "download"  => args.Length > 2 && DateOnly.TryParseExact(args[2], "yyyyMMdd", out DateOnly dl)
                       ? DeviceCommand.Download(dl) : null,
    "sysreset"  => DeviceCommand.BusSysReset(),
    "set"       => args.Length > 4 && ushort.TryParse(args[4], out ushort sv)
                       ? DeviceCommand.SetConfig(args[2].ToUpperInvariant(), args[3].ToUpperInvariant(), sv) : null,
    _           => null
};

if (response is null)
{
    Console.Error.WriteLine($"Unknown or malformed command: {string.Join(' ', args[1..])}");
    return 3;
}

device.RawLineReceived += line => Console.WriteLine(line);
device.SendCommand(response);
await Task.Delay(2000);  // allow response to arrive
return 0;
