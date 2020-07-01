// Compile with "cl /c /Zi /GR- FixupsTest.cpp" on amd64
// Link with "link FixupsTest.obj /debug /debugtype:cv,fixup /nodefaultlib /entry:main"

int IntGlobalVar;

// IMAGE_REL_AMD64_ADDR64         |    0x0 |     0x3000 |     0x3008
int *PtrToGlobal = &IntGlobalVar;

int Func(int IntParam) {
  return IntParam;
}

int main(int argc, char **argv) {
  // IMAGE_REL_AMD64_REL32          |    0x0 |     0x101F |     0x3008
  // IMAGE_REL_AMD64_REL32          | 0x8000 |     0x1024 |     0x1000
  Func(IntGlobalVar);
  // IMAGE_REL_AMD64_REL32          |    0x0 |     0x102B |     0x3000
  // IMAGE_REL_AMD64_REL32          | 0x8000 |     0x1032 |     0x1000
  Func(*PtrToGlobal);
  return 0;
}
