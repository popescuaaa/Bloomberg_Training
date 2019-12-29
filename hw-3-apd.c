#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#define PGM 5
#define PNM 6
/**
 *  Type definitions
 * 
 **/ 
typedef struct 
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;

} pixel;

typedef struct
{
    int height;
    int width;
    int max_val;
    int type;
    /**
     *  For PGM images
     **/ 
    unsigned char **image;
    /**
     *  For PNM images
     **/ 
    pixel **color_image;

} Image;

typedef struct 
{
    char name[50];
    float values[3][3];

} filter;


/**
 *  Const values
 *  ~ Filters ~  
 **/ 
const filter smooth = 
{
    _name = "smooth",
    _values = 
    {
        { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
        { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
        { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
    }
};

const filter aproximative_gaussian_blur = 
{
    _name = "blur",
    _values = 
    {
        { 1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0},
        { 2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0},
        { 1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0}
    }
};

const filter sharpen = 
{
    _name = "sharpen",
    _values = 
    {
        { 0.0, -2.0 / 3.0, 0.0},
        { -2.0 / 3.0, 11.0 / 3.0, -2.0 / 3.0},
        { 0.0, -2.0 / 3.0, 0.0}
    }
};

const filter mean_removal = 
{
    _name = "mean",
    _values = 
    {
        { -1.0, -1.0, -1.0},
        { -1.0, 9.0, -1.0},
        { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
    }
};

const filter embross =
{
    _name = "embross",
    _values = 
    {
        { 0.0, 1.0, 0.0},
        { 0.0, 0.0, 0.0},
        { 0.0, -1.0, 0.0}
    }
};

const filter default_filter = 
{
    _name = "default",
    _values = 
    {
        { 0.0, 0.0, 0.0},
        { 0.0, 1.0, 0.0},
        { 0.0, 0.0, 0.0}
    }
};

/**
 * @param: filter_name 
 * The function return based on name, the associated filter struct.
 * 
 **/

const filter get_filter_by_name(char *filter_name)
{
    if (strcmp(filter_name, "smooth") == 0) return smooth;
    if (strcmp(filter_name, "blur") == 0) return aproximative_gaussian_blur;
    if (strcmp(filter_name, "mean") == 0) return mean_removal;
    if (strcmp(filter_name, "sharpen") == 0) return sharpen;
    if (strcmp(filter_name, "embross") == 0) return embross;

    return default_filter;
}

/**
 * @param: image_file_name
 * Function that reads the content of the image and aditional 
 * data from the indicated filename.
 * 
 **/ 
Image read_image(char *image_file_name)
{
    FILE *fin = fopen(image_file_name, "rb");
    if(fin == NULL)
    {
        printf("The file can't be opened!\n");
        exit(1);
    }

    Image *image = (Image *) malloc (sizeof(Image));
    unsigned char image_type[3];
    unsigned char comment[45];
    int width;
    int height;
    int max_val;
    fscanf(fin, "%s\n", image_type);
    if (strcmp(image_type, "P5") == 0)
    {
        image -> type = PGM;
    } 
    else
    {
        image -> type = PNM;
    }
    fscanf(fin, "%[^\n]%*c", comment);
    fscanf(fin, "%d %d\n", &width, &height);
    fscanf(fin, "%d\n", &max_val);
    image -> width = width;
    image -> height = height;
    image -> max_val = max_val;
    if (image -> type == PGM)
    {
        /**
         *  Read black - white pixels
         **/ 
        unsigned char **pgm_content = (unsigned char **) malloc (image -> height * sizeof(unsigned char *));
        for (int line = 0; line < image -> height; ++line)
        {
            pgm_content[line] = (unsigned char *) malloc(image -> width * sizeof(unsigned char));
        }

        for (int line  = 0; line < image -> height; ++line)
        {
            fread(pgm_content[line], sizeof(unsigned char), image -> width, fin);
        }

        image -> image = pgm_content;
    } 
    else 
    {
        /**
         *  Read RGB pixels
         **/ 
        pixel **pnm_content = (pixel **) malloc (image -> height * sizeof(pixel));
        for (int line = 0; line < image -> height; ++line)
        {
            pnm_content[line] = (pixel *) malloc( image -> width * sizeof(pixel)); 
        }
        for (int line = 0; line < image -> height; ++line)
        {
            fread(pnm_content[line], sizeof(pixel), image -> width, fin);
        }

        image -> color_image = pnm_content;
    }

    fclose(fin);
    return image;
}
/**
 * @param: image
 * @param: output_file_name
 * Write the image in the file specified by name;
 * 
 **/ 
void write_image(Image *image, char *output_file_name)
{
    FILE *fout = fopen(output_file_name, "wb");
    if (fout == NULL)
    {
        printf("The file can't be opened!\n");
        exit(1);
    }
    unsigned char image_type[3];
    unsigned char width[5];
    unsigned char height[5];
    unsigned char max_val[5];
    unsigned char delimiter = " ";
    unsigned char separator = "\n";
    sprintf(width, "%d", image -> width);
    sprintf(height, "%d", image -> height);
    sprintf(max_val, "%d", image -> max_val);

    if (image -> type == PGM)
    {
        image_type = "P5\n";
    } 
    else
    {
        image_type = "P6\n";
    }
    
    fwrite(image_type, sizeof(unsigned char), strlen(image_type), fout);
    fwrite(width, sizeof(unsigned char), strlen(width), fout);
    fwrite(&delimiter, sizeof(unsigned char), 1, fout);
    fwrite(height, sizeof(unsigned char), strlen(height), fout);
    fwrite(&separator, sizeof(unsigned char), 1, fout);
    fwrite(max_val, sizeof(unsigned char), strlen(max_val), fout);
    fwrite(&separator, sizeof(unsigned char), 1, fout);
    if (image -> type == PGM)
    {
        for (int line = 0; line < image -> height; ++line)
        {
            fwrite(image -> image[line], sizeof(unsigned char), image -> width, fout);
        }
    } 
    else
    {
         for (int line = 0; line < image -> height; ++line)
        {
            fwrite(image -> color_image[line], sizeof(pixel), image -> width, fout);
        }
    }
    
    fclose(fout);

    printf("\t\nThe result image has been writen in the current folder: %s\n", output_file_name);
}


int main(int argc, char const *argv[])
{
    
    return 0;
}

