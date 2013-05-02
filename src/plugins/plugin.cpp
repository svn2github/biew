#include "plugin.h"
#include "codeguid.h"
#include "reg_form.h"
#include "disasm.h"
#include "beyeutil.h"

#include "bmfile.h"

namespace beye {
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

const char* Bin_Format::name() const { return detectedFormat->name; }
const char* Bin_Format::prompt(unsigned idx) const { return detectedFormat->prompt[idx]; }
__filesize_t Bin_Format::action_F1() const { return detectedFormat->action[0]?detectedFormat->action[0]():Bad_Address; }
__filesize_t Bin_Format::action_F2() const { return detectedFormat->action[1]?detectedFormat->action[1]():Bad_Address; }
__filesize_t Bin_Format::action_F3() const { return detectedFormat->action[2]?detectedFormat->action[2]():Bad_Address; }
__filesize_t Bin_Format::action_F4() const { return detectedFormat->action[3]?detectedFormat->action[3]():Bad_Address; }
__filesize_t Bin_Format::action_F5() const { return detectedFormat->action[4]?detectedFormat->action[4]():Bad_Address; }
__filesize_t Bin_Format::action_F6() const { return detectedFormat->action[5]?detectedFormat->action[5]():Bad_Address; }
__filesize_t Bin_Format::action_F7() const { return detectedFormat->action[6]?detectedFormat->action[6]():Bad_Address; }
__filesize_t Bin_Format::action_F8() const { return detectedFormat->action[7]?detectedFormat->action[7]():Bad_Address; }
__filesize_t Bin_Format::action_F9() const { return detectedFormat->action[8]?detectedFormat->action[8]():Bad_Address; }
__filesize_t Bin_Format::action_F10() const { return detectedFormat->action[9]?detectedFormat->action[9]():Bad_Address; }

__filesize_t Bin_Format::show_header() const {
    if(detectedFormat->showHdr) return detectedFormat->showHdr(); /**< if not an MZ style format */
    if(IsNewExe()) return mz_format?mz_format->showHdr():Bad_Address;
}
bool Bin_Format::bind(const DisMode& _parent,char *str,__filesize_t shift,int flg,int codelen,__filesize_t r_shift) const {
    return detectedFormat->bind?detectedFormat->bind(_parent,str,shift,flg,codelen,r_shift):false;
}
int Bin_Format::query_platform() const { return detectedFormat->query_platform?detectedFormat->query_platform():DISASM_DEFAULT; }
int Bin_Format::query_bitness(__filesize_t off) const { return detectedFormat->query_bitness?detectedFormat->query_bitness(off):DAB_USE16; }
int Bin_Format::query_endian(__filesize_t off) const { return detectedFormat->query_endian?detectedFormat->query_endian(off):DAE_LITTLE; }
bool Bin_Format::address_resolving(char * str,__filesize_t off) const { return detectedFormat->AddressResolving?detectedFormat->AddressResolving(str,off):false; }
__filesize_t Bin_Format::va2pa(__filesize_t va) const {
    __filesize_t rc=Bad_Address;
    if(detectedFormat->va2pa) {
	rc=detectedFormat->va2pa(va);
	if(!rc) rc=Bad_Address;
    }
    return rc;
}
__filesize_t Bin_Format::pa2va(__filesize_t pa) const {
    __filesize_t rc=Bad_Address;
    if(detectedFormat->pa2va) {
	rc=detectedFormat->pa2va(pa);
	if(!rc) rc=Bad_Address;
    }
    return rc;
}
__filesize_t Bin_Format::get_public_symbol(char *str,unsigned cb_str,unsigned *_class,__filesize_t pa,bool as_prev) const {
    __filesize_t rc=Bad_Address;
    if(detectedFormat->GetPubSym) {
	rc=detectedFormat->GetPubSym(str,cb_str,_class,pa,as_prev);
	if(!rc) rc = Bad_Address;
    }
    return rc;
}
unsigned Bin_Format::get_object_attribute(__filesize_t pa,char *name,unsigned cb_name,__filesize_t *start,__filesize_t *end,int *_class,int *bitness) const {
    return detectedFormat->GetObjAttr?detectedFormat->GetObjAttr(pa,name,cb_name,start,end,_class,bitness):0;
}
} // namespace beye
