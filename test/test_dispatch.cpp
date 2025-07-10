/*
Unit tests for dispatching.
1. Nearest dispatch should dispatch the closest fire station to the incident.
2. Firebeats should follow the zoning rules.
3. Dispatch should not dispatch if there are no available fire stations.
4. Dispatch should handle multiple incidents correctly.
5. Dispatch should prioritize incidents based on severity.****
    - this is not yet certain. Right now its priority based on first come first served basis.
6. Dispatch should log all actions taken.
7. Dispatch should handle edge cases like no incidents or all stations busy.
8. Dispatch should handle concurrent incidents correctly.
9. Dispatch should ensure that the nearest station is not already responding to another incident.
*/