#include "png.hh"

png_t::png_t() {
}

png_t::~png_t() {
}

void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

void png_t::read_file(std::string filename) {
    unsigned char header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(filename.c_str(), "rb");
    if (!fp)
            abort_("[read_png_file] File %s could not be opened for reading", filename.c_str());
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8))
            abort_("[read_png_file] File %s is not recognized as a PNG file", filename.c_str());


    /* initialize stuff */
    png_ptr_ = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr_)
            abort_("[read_png_file] png_create_read_struct failed");

    info_ptr_ = png_create_info_struct(png_ptr_);
    if (!info_ptr_)
            abort_("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr_)))
            abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr_, fp);
    png_set_sig_bytes(png_ptr_, 8);

    png_read_info(png_ptr_, info_ptr_);

    width_ = png_get_image_width(png_ptr_, info_ptr_);
    height_ = png_get_image_height(png_ptr_, info_ptr_);
    color_type_ = png_get_color_type(png_ptr_, info_ptr_);
    bit_depth_ = png_get_bit_depth(png_ptr_, info_ptr_);

    number_of_passes_ = png_set_interlace_handling(png_ptr_);
    png_read_update_info(png_ptr_, info_ptr_);


    /* read file */
    if (setjmp(png_jmpbuf(png_ptr_)))
            abort_("[read_png_file] Error during read_image");

    row_pointers_ = (png_bytep*) malloc(sizeof(png_bytep) * height_);
    for (int y=0; y<height_; y++)
            row_pointers_[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr_,info_ptr_));

    png_read_image(png_ptr_, row_pointers_);

    fclose(fp);
}

png_byte* png_t::row(uint16_t row_number) {
	return row_pointers_[row_number];
}

