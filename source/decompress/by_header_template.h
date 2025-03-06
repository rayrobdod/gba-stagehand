static UnCompFn CAT(UnCompFn, XRAM)(unsigned magic) {
	switch (magic) {
	case 0x00:
		return &IdentityUnComp;
	case 0x10:
		return &CAT(LZ77UnComp, XRAM);
	case 0x24:
	case 0x28:
		return &HuffUnComp;
	case 0x30:
		return &CAT(RLUnComp, XRAM);
	case 0x81:
		return &CAT(Diff8UnFilter, XRAM);
	case 0x82:
		return &Diff16UnFilter;
	default:
		return NULL;
	}
}

void CAT(HeaderUnComp, XRAM)(const void* src, volatile void* dest) {
	const uint8_t* src8 = (const uint8_t*)src;
	const uint8_t magic = src8[0];
	UnCompFn fn = CAT(UnCompFn, XRAM)(magic);

	if (fn) {
		fn(src, dest);
	} else {
		MgbaPrintf(MGBA_LOG_FATAL, "Unknown compression magic: %02x (%p -> %p)", magic, src, dest);
	}
}
