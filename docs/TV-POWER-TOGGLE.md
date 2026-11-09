# TV / Monitor Power Toggle

## Approved behaviour
A single **POWER TOGGLE** button for the user's selected, authorised television or monitor profile. A toggle is not a guaranteed OFF command: if the display is already off, it may turn on.

## Profile import
Index Flipper-IRDB .ir commands named Power, Power_toggle, PowerToggle or equivalent only after inspecting their protocol and verifying they are actual power toggles. Do not treat Power_on or Power_off as toggles. Never fabricate codes.

## Execution
1. Select and save one compatible remote profile.
2. Require a deliberate tap on POWER TOGGLE.
3. Validate the selected profile and IR command.
4. Transmit once through onboard IR TX using a supported, verified protocol.
5. Show a truthful success/failure response; transmission does not confirm the display changed state.

Do not broadcast every code in the database or operate unrelated displays. No automatic repeats, scheduled transmission or ambient sweeps.

## Status
Specification committed; firmware transmission dispatcher and dashboard integration are not yet implemented or tested.
