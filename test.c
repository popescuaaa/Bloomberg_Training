#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


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

/**
 * @param: image_file_name
 * Function that reads the content of the image and aditional 
 * data from the indicated filename.
 * 
 **/ 
Image *read_image(char *image_file_name)
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
        image -> type = 5;
    } 
    else
    {
        image -> type = 6;
    }
   // fscanf(fin, "%[^\n]%*c", comment);
    fscanf(fin, "%d %d\n", &width, &height);
    fscanf(fin, "%d\n", &max_val);
    image -> width = width;
    image -> height = height;
    image -> max_val = max_val;
    if (image -> type == 5)
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
        pixel **pnm_content = (pixel **) malloc (image -> height * sizeof(pixel *));
        for (int line = 0; line < image -> height; ++line)
        {
            pnm_content[line] = (pixel *) malloc (image -> width * sizeof(pixel)); 
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
    unsigned char *image_type;
    unsigned char width[5];
    unsigned char height[5];
    unsigned char max_val[5];
    unsigned char delimiter = ' ';
    unsigned char separator = '\n';
    sprintf(width, "%d", image -> width);
    sprintf(height, "%d", image -> height);
    sprintf(max_val, "%d", image -> max_val);

    if (image -> type == 5)
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
    if (image -> type == 5)
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

/**
 *  Main entry of the program
 * 
 * 
 **/ 
int main(int argc, char *argv[])
{
    Image *before = read_image(argv[3]);
    Image *in = read_image(argv[1]);
    Image *ref = read_image(argv[2]);

    for (int i = in -> height /4; i < (in -> height /4 ) + 3; i++)
    {
        for (int j = in -> width / 2; j < (in -> width / 2) + 3; j++)
        {
            printf("%u ", in -> image[i][j]);
        }
        printf("\n");
    }

    
     for (int i = in -> height / 4; i < (in -> height /4 ) + 3; i++)
    {
        for (int j = in -> width / 2; j < (in -> width / 2) + 3; j++)
        {
            printf("%u ", ref -> image[i][j]);
        }
        printf("\n");
    }

     for (int i = in -> height / 4; i < (in -> height /4 ) + 3; i++)
    {
        for (int j = in -> width / 2; j < (in -> width / 2) + 3; j++)
        {
            printf("%u ", before -> image[i][j]);
        }
        printf("\n");
    }

    // for (int i = 0; i < in -> height; i++)
    // {
    //     for (int j = 0; j < in -> width; j++)
    //     {
    //         if (in -> image[i][j] != ref -> image[i][j])
    //         {
    //             printf("In: %u Ref: %u\n\n", in -> image[i][j], ref -> image[i][j]);
    //         }
    //     }
    // }
    return 0;
}
