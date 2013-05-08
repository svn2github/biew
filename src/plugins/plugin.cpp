#include "plugin.h"
#include "codeguid.h"
#include "reg_form.h"
#include "disasm.h"
#include "beyeutil.h"

#include "bmfile.h"

namespace	usr {
extern const REGISTRY_BIN binTable;
extern const REGISTRY_BIN rmTable;
extern const REGISTRY_BIN movTable;
extern const REGISTRY_BIN mp3Table;
extern const REGISTRY_BIN mpegTable;
extern const REGISTRY_BIN jpegTable;
extern const REGISTRY_BIN wavTable;
extern const REGISTRY_BIN aviTable;
extern const REGISTRY_BIN asfTable;
extern const REGISTRY_BIN bmpTable;
extern const REGISTRY_BIN neTable;
extern const REGISTRY_BIN peTable;
extern const REGISTRY_BIN leTable;
extern const REGISTRY_BIN lxTable;
extern const REGISTRY_BIN nlm386Table;
extern const REGISTRY_BIN elf386Table;
extern const REGISTRY_BIN jvmTable;
extern const REGISTRY_BIN coff386Table;
extern const REGISTRY_BIN archTable;
extern const REGISTRY_BIN aoutTable;
extern const REGISTRY_BIN OldPharLapTable;
extern const REGISTRY_BIN PharLapTable;
extern const REGISTRY_BIN rdoffTable;
extern const REGISTRY_BIN rdoff2Table;
extern const REGISTRY_BIN sisTable;
extern const REGISTRY_BIN sisxTable;
extern const REGISTRY_BIN lmfTable;
extern const REGISTRY_BIN mzTable;
extern const REGISTRY_BIN dossysTable;

Bin_Format::Bin_Format(CodeGuider& _parent)
	    :parent(_parent)
{
    formats.push_back(&neTable);
    formats.push_back(&peTable);
    formats.push_back(&leTable);
    formats.push_back(&lxTable);
    formats.push_back(&nlm386Table);
    formats.push_back(&elf386Table);
    formats.push_back(&jvmTable);
    formats.push_back(&coff386Table);
    formats.push_back(&archTable);
    formats.push_back(&aoutTable);
    formats.push_back(&OldPharLapTable);
    formats.push_back(&PharLapTable);
    formats.push_back(&rdoffTable);
    formats.push_back(&rdoff2Table);
    formats.push_back(&lmfTable);
    formats.push_back(&sisTable);
    formats.push_back(&sisxTable);
    formats.push_back(&aviTable);
    formats.push_back(&asfTable);
    formats.push_back(&bmpTable);
    formats.push_back(&mpegTable);
    formats.push_back(&jpegTable);
    formats.push_back(&wavTable);
    formats.push_back(&movTable);
    formats.push_back(&rmTable);
    formats.push_back(&mp3Table);
    formats.push_back(&mzTable);
    formats.push_back(&dossysTable);
    formats.push_back(&binTable);
    detectedFormat = &binTable;
    mz_format = &mzTable;
}
Bin_Format::~Bin_Format() { if(detectedFormat->destroy) detectedFormat->destroy(); }

void Bin_Format::detect_format()
{
    if(!bmGetFLength()) return;
    size_t i,sz=formats.size();
    for(i = 0;i < sz;i++) {
	if(formats[i]->check_format()) {
	    detectedFormat = formats[i];
	    if(detectedFormat->init) detectedFormat->init(parent);
	    break;
	}
    }
}

int	Bin_Format::query_platform() const { return detectedFormat->query_platform?detectedFormat->query_platform():DISASM_DEFAULT; }
int	Bin_Format::query_bitness(__filesize_t off) const { return detectedFormat->query_bitness?detectedFormat->query_bitness(off):DAB_USE16; }
int	Bin_Format::query_endian(__filesize_t off) const { return detectedFormat->query_endian?detectedFormat->query_endian(off):DAE_LITTLE; }

} // namespace	usr
