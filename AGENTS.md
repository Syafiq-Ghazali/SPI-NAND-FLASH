# Project guidance

## Code and comments

- Code is the source of truth. Prefer clear names, small functions, and explicit behavior.
- Keep comments minimal. Explain only non-obvious intent, hardware constraints, or tradeoffs.
- Do not add file banners, decorative section dividers, function documentation templates, or comments that repeat the code.
- Keep essential API contracts short and close to their declarations.
- Remove redundant comments when touching code; avoid unrelated formatting churn.
- Keep documentation focused on setup, usage, and constraints callers must understand. Link to code instead of duplicating implementation details.

## Implementation

- Preserve the layers: BSW for MCU peripherals, BRD for board configuration, MSW for the NAND driver, and ASW for application behavior.
- Keep pins, clocks, and device configuration in BRD.
- Use the existing types and naming conventions. On C2000, buffers use Uint16_t elements for individual transferred octets; do not assume an 8-bit C byte.
- Respect the device's ECC, bad-block, erase, and programming requirements. Make responsibility for rules enforced by callers explicit.
- Bound hardware waits, propagate failures, and define recovery after interrupted operations.
- Prefer the smallest implementation that solves the current problem. Add abstractions and dependencies only when needed.
- The planned Python interface should expose a reusable client library with a thin CLI. Keep transport separate from flash operations and document the shared protocol.

## Validation and delivery

- Run checks appropriate to the change. For firmware behavior, use a target build and relevant hardware checks when available.
- Report what was verified and what still needs hardware validation. Never imply an unperformed check passed.
- Keep commits focused. Exclude generated build artifacts and unrelated user changes.
