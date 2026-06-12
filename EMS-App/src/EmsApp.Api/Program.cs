using EmsApp.Serial;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddSingleton<DeviceConnection>(sp =>
{
    string port = builder.Configuration["Device:Port"] ?? "COM3";
    int baud    = int.TryParse(builder.Configuration["Device:BaudRate"], out int b) ? b : 115200;
    return new DeviceConnection(port, baud);
});
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

var app = builder.Build();
app.UseSwagger();
app.UseSwaggerUI();

// Open device connection on startup
var device = app.Services.GetRequiredService<DeviceConnection>();
device.Open();

app.MapGet("/status", () => Results.Ok(new { connected = device.IsOpen }));

app.MapPost("/command/time", (DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.SetTime(DateTimeOffset.UtcNow));
    return Results.Ok();
});

app.MapPost("/command/devid/{id}", (ushort id, DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.SetDeviceId(id));
    return Results.Ok();
});

app.MapPost("/command/aircraft/{id}", (ushort id, DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.SetAircraftId(id));
    return Results.Ok();
});

app.MapPost("/command/speed/{kbps}", (int kbps, DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.SetCanSpeed(kbps));
    return Results.Ok();
});

app.MapPost("/command/clear", (DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.ClearStore());
    return Results.Ok();
});

app.MapPost("/command/reset", (DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.Reset());
    return Results.Ok();
});

app.MapGet("/command/list", (DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.ListStore());
    return Results.Accepted();
});

app.MapGet("/download/{date}", (string date, DeviceConnection dev) =>
{
    if (!DateOnly.TryParseExact(date, "yyyyMMdd", out DateOnly d))
        return Results.BadRequest("Date must be yyyyMMdd");
    dev.SendCommand(EmsApp.Core.DeviceCommand.Download(d));
    return Results.Accepted();
});

app.MapPost("/command/sysreset", (DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.BusSysReset());
    return Results.Ok();
});

// SET:<TYPE>:<PARAM>:<VALUE> — param mnemonics: see docs/can-param-mnemonics.md
app.MapPost("/command/set/{type}/{param}/{value}", (string type, string param, ushort value, DeviceConnection dev) =>
{
    dev.SendCommand(EmsApp.Core.DeviceCommand.SetConfig(type.ToUpperInvariant(), param.ToUpperInvariant(), value));
    return Results.Ok();
});

app.Run();
