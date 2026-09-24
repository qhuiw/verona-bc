# VBC Emitter Coverage

**Current:** VBC is the default VC output and the general textual-VIR output.
The `vc.cmake` and `vbc.cmake` collections compile fixtures, compare
goldens, and execute successful bytecode with VBCI.

Format-sensitive behavior is covered by textual VIR fixtures and by the broad
source fixture suite. Runtime-only VBCI behavior remains owned by interpreter
tests rather than this emitter.