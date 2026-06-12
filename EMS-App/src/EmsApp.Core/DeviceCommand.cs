namespace EmsApp.Core;

public static class DeviceCommand
{
    public static string SetTime(DateTimeOffset dt)         => $"TIME:{dt.ToUnixTimeSeconds()}";
    public static string SetDeviceId(ushort id)             => $"DEVID:{id:X4}";
    public static string SetAircraftId(ushort id)           => $"AIRCRAFT:{id:X4}";
    public static string SetCanSpeed(int kbps)              => $"SPEED:{kbps}";
    public static string ClearStore()                       => "CLEAR";
    public static string ListStore()                        => "LIST";
    public static string Reset()                            => "RESET";
    public static string Download(DateOnly date)            => $"DOWNLOAD:{date:yyyyMMdd}";

    // CAN bus commands (forwarded over the bus by the monitor)
    public static string BusSysReset()                                               => "SYSRESET";
    public static string SetConfig(string type, string param, ushort value)          => $"SET:{type}:{param}:{value}";

    // Typed helpers — param mnemonics: see docs/can-param-mnemonics.md
    public static string SetRpmGreenLo(ushort rpm)  => SetConfig("RPM", "GRL", rpm);
    public static string SetRpmGreenHi(ushort rpm)  => SetConfig("RPM", "GRH", rpm);
    public static string SetRpmRedLine(ushort rpm)  => SetConfig("RPM", "RED", rpm);
    public static string SetAltQnh(ushort hpa)      => SetConfig("ALT", "QNH", hpa);
    public static string SetEgtCautionLo(ushort v)  => SetConfig("EGT", "CAL", v);
    public static string SetEgtCautionHi(ushort v)  => SetConfig("EGT", "CAH", v);
    public static string SetChtCautionLo(ushort v)  => SetConfig("CHT", "CAL", v);
    public static string SetChtCautionHi(ushort v)  => SetConfig("CHT", "CAH", v);
}
