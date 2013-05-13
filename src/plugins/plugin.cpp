#include "plugin.h"
#include "codeguid.h"
#include "reg_form.h"
#include "disasm.h"
#include "beyeutil.h"
#include "beye.h"
#include "libbeye/bstream.h"

namespace	usr {
extern const Binary_Parser_Info bin_info;
extern const Binary_Parser_Info rm_info;
extern const Binary_Parser_Info mov_info;
extern const Binary_Parser_Info mp3_info;
extern const Binary_Parser_Info mpeg_info;
extern const Binary_Parser_Info jpeg_info;
extern const Binary_Parser_Info wav_info;
extern const Binary_Parser_Info avi_info;
extern const Binary_Parser_Info asf_info;
extern const Binary_Parser_Info bmp_info;
extern const Binary_Parser_Info ne_info;
extern const Binary_Parser_Info pe_info;
extern const Binary_Parser_Info le_info;
extern const Binary_Parser_Info lx_info;
extern const Binary_Parser_Info nlm_info;
extern const Binary_Parser_Info elf_info;
extern const Binary_Parser_Info jvm_info;
extern const Binary_Parser_Info coff_info;
extern const Binary_Parser_Info arch_info;
extern const Binary_Parser_Info aout_info;
extern const Binary_Parser_Info oldpharlap_info;
extern const Binary_Parser_Info pharlap_info;
extern const Binary_Parser_Info rdoff_info;
extern const Binary_Parser_Info rdoff2_info;
extern const Binary_Parser_Info sis_info;
extern const Binary_Parser_Info sisx_info;
extern const Binary_Parser_Info lmf_info;
extern const Binary_Parser_Info mz_info;
extern const Binary_Parser_Info dossys_info;

Bin_Format::Bin_Format(CodeGuider& _parent)
	    :parent(_parent)
{
    formats.push_back(&ne_info);
    formats.push_back(&pe_info);
    formats.push_back(&le_info);
    formats.push_back(&lx_info);
    formats.push_back(&nlm_info);
    formats.push_back(&elf_info);
    formats.push_back(&jvm_info);
    formats.push_back(&coff_info);
    formats.push_back(&arch_info);
    formats.push_back(&aout_info);
    formats.push_back(&oldpharlap_info);
    formats.push_back(&pharlap_info);
    formats.push_back(&rdoff_info);
    formats.push_back(&rdoff2_info);
    formats.push_back(&lmf_info);
    formats.push_back(&sis_info);
    formats.push_back(&sisx_info);
    formats.push_back(&avi_info);
    formats.push_back(&asf_info);
    formats.push_back(&bmp_info);
    formats.push_back(&mpeg_info);
    formats.push_back(&jpeg_info);
    formats.push_back(&wav_info);
    formats.push_back(&mov_info);
    formats.push_back(&rm_info);
    formats.push_back(&mp3_info);
    formats.push_back(&mz_info);
    formats.push_back(&dossys_info);
    formats.push_back(&bin_info);
}
Bin_Format::~Bin_Format() { delete detectedFormat; }

void Bin_Format::detect_format()
{
    if(!beye_context().sc_bm_file().flength()) return;
    size_t i,sz=formats.size();
    for(i = 0;i < sz;i++) {
	if(formats[i]->probe()) {
	    detectedFormat = formats[i]->query_interface(parent);
	    break;
	}
    }
}

int	Bin_Format::query_platform() const { return detectedFormat->query_platform(); }
int	Bin_Format::query_bitness(__filesize_t off) const { return detectedFormat->query_bitness(off); }
int	Bin_Format::query_endian(__filesize_t off) const { return detectedFormat->query_endian(off); }

} // namespace	usr
