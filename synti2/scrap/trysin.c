/* Some inline assembler for i387 sin. */
static
float asm_sin(float t){
  float result;
  asm(
    "fld %1\n"
    "fsin\n"
    "fwait\n"
    "fstp %0\n"
    "fwait\n"
    : "=m"(result)
    : "m"(t)
      );
  return result;
}

int main(int argc, char **argv){
  float a = 2.0f;
  float b;
  //b = sin(a);
  b = asm_sin(a);
  printf("Ja tuota %f\n",b);
  return b;
}
