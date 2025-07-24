struct CompressedData;

void LZ11UnCompVram(const struct CompressedData* src, volatile void* dest);
void LZ11UnCompWram(const struct CompressedData* src, volatile void* dest);
