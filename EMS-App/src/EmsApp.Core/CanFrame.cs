namespace EmsApp.Core;

public record CanFrame(
    uint   CanId,
    byte   Length,
    byte[] Data,
    string Timestamp,
    ushort AircraftId
);
