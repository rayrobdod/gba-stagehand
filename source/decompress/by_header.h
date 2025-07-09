struct CompressedData;

void HeaderUnCompWram(const struct CompressedData* src, volatile void* dest);
void HeaderUnCompVram(const struct CompressedData* src, volatile void* dest);
