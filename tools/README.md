# tools

`test_pure_logic.cpp` checks the helpers that do not depend on Windows or on the game:
`PatternMatch.h` (byte pattern matching) and `StringUtils` (hex parsing and address formatting).

`StringUtils.cpp` includes the Windows precompiled header, so on a non Windows host build it from
a copy without that include:

```bash
mkdir -p /tmp/dsdm
sed 's|#include "pch.h"||' StringUtils.cpp | sed '1s/^\xEF\xBB\xBF//' > /tmp/dsdm/StringUtils.cpp
cp StringUtils.h PatternMatch.h /tmp/dsdm/
g++ -std=c++17 -fsanitize=address,undefined -I/tmp/dsdm tools/test_pure_logic.cpp /tmp/dsdm/StringUtils.cpp -o /tmp/dsdm/test_pure_logic
/tmp/dsdm/test_pure_logic
```

It prints `ALL OK` and exits with 0 when every check passes.

The memory code (`MemoryUtils`, the region walk in `PatternScanner`) needs a Windows process and is
validated manually, see the checklist in the main README.
