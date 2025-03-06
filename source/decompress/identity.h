/**

A compression format that doesn't perform compression,
but does have a header for `by_header` to disambiguate over
and that contains the data length

header:
	bits 0-7: the magic number 0x00
	bits 8-31: uncompressed length
data:
	the uncompressed data

*/
void IdentityUnComp(const void* src, volatile void* dest);
