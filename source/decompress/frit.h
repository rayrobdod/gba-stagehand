struct CompressedData;

void Frit16UnComp(const struct CompressedData* src, volatile void* dest);
void Frit8UnCompVram(const struct CompressedData* src, volatile void* dest);
void Frit8UnCompWram(const struct CompressedData* src, volatile void* dest);
