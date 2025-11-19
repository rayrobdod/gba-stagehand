static UnCompFn CAT(UnCompFn, XRAM)(unsigned magic) {
	switch (magic) {
	case 0x00:
		return &IdentityUnComp;
	case 0x10:
		return &CAT(LZ77UnComp, XRAM);
	case 0x11:
		return &CAT(LZ11UnComp, XRAM);
	case 0x24:
	case 0x28:
		return &HuffUnComp;
	case 0x30:
		return &CAT(RLUnComp, XRAM);
	case 0x31:
		return &RlZeroUnComp;
	case 0x41:
		return &CAT(Frit8UnComp, XRAM);
	case 0x42:
		return &Frit16UnComp;
	case 0x81:
		return &CAT(Diff8UnFilter, XRAM);
	case 0x82:
		return &Diff16UnFilter;
	case 0xF1:
	case 0xF2:
	case 0xF3:
	case 0xF4:
	case 0xF5:
	case 0xF6:
	case 0xF7:
		return &SmolUnComp;
	default:
		return NULL;
	}
}

void CAT(HeaderUnComp, XRAM)(const struct CompressedData* src, volatile void* dest) {
	const uint8_t magic = src->magic;
	UnCompFn fn = CAT(UnCompFn, XRAM)(magic);

	if (fn) {
		fn(src, dest);
	} else {
		MgbaPrintf(MGBA_LOG_FATAL, "Unknown compression magic: %02x (%p -> %p)", magic, src, dest);
	}
}
