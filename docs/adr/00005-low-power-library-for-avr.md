# 00005. Low power library for AVR

2024-06-14

## Status

__New__

## Context

The low power library is not suitable for AVR boards. So using the low power library is not useable.

## Decision

Keep the library in mind for other projects.

## Consequences

Wait cycles are handled with the `delayMicroseconds( n )` method.
