# Test Pipelines

Top-level collection files register CTest dependency graphs by input and output
pipeline:

| Collection | Pipeline |
| --- | --- |
| `vc-vbc.cmake` | Verona source -> VC -> VBC -> VBCI |
| `vc-llvm.cmake` | Allowlisted Verona source -> VC -> LLVM -> native VRT |
| `vir-vbc.cmake` | Textual VIR -> VIRC -> VBC -> VBCI |
| `vir-llvm.cmake` | Allowlisted VIR -> VIRC -> LLVM -> native VRT |
| `vrt.cmake` | VRT ABI and runtime fixtures |

LLVM collections return without registering tests when
`VERONA_ENABLE_LLVM_BACKEND=OFF`. Enabling LLVM adds explicit native coverage
and never changes the default VBC pipeline.

CTest node names remain stable independently of collection filenames. Build
and run from the canonical `build/` directory with `ninja install` followed by
`ctest --output-on-failure`.