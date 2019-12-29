#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>

#define PGM 5
#define PNM 6
#define DEFAULT_TAG 0
#define MASTER 0

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
    "smooth", 
    {
        { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
        { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
        { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
    }
};

const filter aproximative_gaussian_blur = 
{
    "blur", 
    {
        { 1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0},
        { 2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0},
        { 1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0}
    }
};

const filter sharpen = 
{
    "sharpen",
    {
        { 0.0, -2.0 / 3.0, 0.0},
        { -2.0 / 3.0, 11.0 / 3.0, -2.0 / 3.0},
        { 0.0, -2.0 / 3.0, 0.0}
    }
};

const filter mean_removal = 
{
    "mean",
    {
        { -1.0, -1.0, -1.0},
        { -1.0, 9.0, -1.0},
        { 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
    }
};

const filter embross =
{
    "embross",
    {
        { 0.0, 1.0, 0.0},
        { 0.0, 0.0, 0.0},
        { 0.0, -1.0, 0.0}
    }
};

const filter default_filter = 
{
    "default", 
    {
        { 0.0, 0.0, 0.0},
        { 0.0, 1.0, 0.0},
        { 0.0, 0.0, 0.0}
    }
};

/**
 * @param: filter_name 
 * The function return based on name, the associated filter struct.
 * It handle also non-valid filter names.
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
    unsigned char *image_type;
    unsigned char width[5];
    unsigned char height[5];
    unsigned char max_val[5];
    unsigned char delimiter = ' ';
    unsigned char separator = '\n';
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

/**
 *  The following functions repesent the API that handles the processes reposnse
 *  and action to specific images, so basically the image processing parallel API.
 * 
 **/ 

/**
 * @param: image
 * @param: destination
 * @param: start
 * @param: finish
 * 
 * Send unsing MPI a couple of lines from a image content to another process.
 * Mainly used by master to scatter the matrix butr also by any slave process 
 * that have to send the processed matrix back.
 * The function send all data about the image and make no assumption for the 
 * receiver.
 **/
void send_image(Image *image, int destination, int start, int finish)
{
    int _height = finish - start;
    MPI_Send(&(image -> type), 1, MPI_INT, destination, DEFAULT_TAG, MPI_COMM_WORLD);
    MPI_Send(&(image -> width), 1, MPI_INT, destination, DEFAULT_TAG, MPI_COMM_WORLD);
    MPI_Send(&(_height), 1, MPI_INT, destination, DEFAULT_TAG, MPI_COMM_WORLD);
    MPI_Send(&(image -> max_val), 1, MPI_INT, destination, DEFAULT_TAG, MPI_COMM_WORLD);
    if (image -> type == PGM)
    {
        for (int line = start; line < finish; ++line)
        {
            MPI_Send(image -> image[line], image -> width, MPI_UNSIGNED_CHAR, destination, DEFAULT_TAG, MPI_COMM_WORLD);
        }
    } 
    else
    {
         for (int line = start; line < finish; ++line)
        {
            MPI_Send(image -> color_image[line], 3 * image -> width, MPI_UNSIGNED_CHAR, destination, DEFAULT_TAG, MPI_COMM_WORLD);
        }
    }
}

/**
 * @param: source
 * 
 * Function that receive an image from a specified source.
 * The function receives all the data previously send by the other function
 * and make no assumption for the sender.
 * He only receives a couple of lines send with the above function.
 **/ 
Image *receive_image(int source)
{
    Image *image = (Image *) malloc(sizeof(Image));
    MPI_Recv(&(image -> type), 1, MPI_INT, source, DEFAULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&(image -> width), 1, MPI_INT, source, DEFAULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&(image -> height), 1, MPI_INT, source, DEFAULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&(image -> max_val), 1, MPI_INT, source, DEFAULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (image -> type == PGM)
    {
        unsigned char **content = (unsigned char **) malloc(image -> height * sizeof(unsigned char *));
        for (int line  = 0; line < image -> height; ++line)
        {
            content[line] =  (unsigned char *) malloc (image -> width * sizeof(unsigned char));
        }
        
        for (int line = 0; line < image -> height; ++line)
        {
            MPI_Recv(content[line], image -> width, MPI_UNSIGNED_CHAR, source, DEFAULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        
        image -> image = content;
    
    } 
    else
    {
        pixel **rgb_content = (pixel **) malloc(image -> height * sizeof(pixel *));
        for (int line = 0; line < image -> height; ++line)
        {
            rgb_content[line] =  (pixel *) malloc(image -> width * sizeof(pixel));
        }

        for (int line = 0; line < image -> height; ++line)
        {
            MPI_Recv(rgb_content[line], 3 * image -> width, MPI_UNSIGNED_CHAR, source, DEFAULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);   
        }

        image -> color_image = rgb_content;
    }
    
    return image;
}
/**
 *  @param: image
 *  @param: filter
 *  @param: start_line -> in interval 0 - end_line
 *  @param: end_line -> in interval 0 - height
 * 
 *  Apply a specific filter on an image in a region delimited by on height 
 *  by start_line and end_line.
 *  It also ensure that there is no overflow or underflow during the matrix
 *  multiplication by clamping values to 0 for begative ones and to 255 
 *  for unsigned char values that goes above the max value.
 * 
 **/
void apply_filter(Image *image, const filter current_filter, int start_line, int end_line)
{
    Image *result = (Image *) malloc(sizeof(Image));
    result -> width = image -> width;
    result -> height = image -> height;
    result -> type = image -> type;
    result -> max_val = image -> max_val;
    
    if (image -> type == PGM)
    {
         for (int line = start_line; line < end_line; ++line)
         {
            for (int column = 0; column < image -> width; ++column)
            {
                if ( (line  == 0) || (column == 0) || (line == image -> height - 1) || (column == image -> width - 1))
                {
                    result -> image[line][column] = image -> image[line][column]; 
                }
                else
                {
                     result -> image[line][column] =
                       current_filter.values[0][0] * image -> image[line - 1][column - 1] +
                       current_filter.values[0][1] * image -> image[line - 1][column] +
                       current_filter.values[0][2] * image -> image[line - 1][column + 1] +
                       current_filter.values[1][0] * image -> image[line][column - 1] +
                       current_filter.values[1][1] * image -> image[line][column] +
                       current_filter.values[1][2] * image -> image[line][column + 1] +
                       current_filter.values[2][0] * image -> image[line + 1][column - 1] +
                       current_filter.values[2][1] * image -> image[line + 1][column] +
                       current_filter.values[2][2] * image -> image[line + 1][column + 1];

                     image -> image[line][column] = result -> image[line][column];
                }   
            }
         }
    }
    else
    {
        for (int line = start_line; line < end_line; ++line)
         {
            for (int column = 0; column < image -> width; ++column)
            {
                if ( (line  == 0) || (column == 0) || (line == image -> height - 1) || (column == image -> width - 1))
                {
                    result -> color_image[line][column] = image -> color_image[line][column]; 
                }
                else
                {
                     result -> color_image[line][column].red =
                       current_filter.values[0][0] * image -> color_image[line - 1][column - 1].red +
                       current_filter.values[0][1] * image -> color_image[line - 1][column].red +
                       current_filter.values[0][2] * image -> color_image[line - 1][column + 1].red +
                       current_filter.values[1][0] * image -> color_image[line][column - 1].red +
                       current_filter.values[1][1] * image -> color_image[line][column].red +
                       current_filter.values[1][2] * image -> color_image[line][column + 1].red +
                       current_filter.values[2][0] * image -> color_image[line + 1][column - 1].red +
                       current_filter.values[2][1] * image -> color_image[line + 1][column].red +
                       current_filter.values[2][2] * image -> color_image[line + 1][column + 1].red;

                    result -> color_image[line][column].green =
                       current_filter.values[0][0] * image -> color_image[line - 1][column - 1].green +
                       current_filter.values[0][1] * image -> color_image[line - 1][column].green +
                       current_filter.values[0][2] * image -> color_image[line - 1][column + 1].green +
                       current_filter.values[1][0] * image -> color_image[line][column - 1].green +
                       current_filter.values[1][1] * image -> color_image[line][column].green +
                       current_filter.values[1][2] * image -> color_image[line][column + 1].green +
                       current_filter.values[2][0] * image -> color_image[line + 1][column - 1].green +
                       current_filter.values[2][1] * image -> color_image[line + 1][column].green +
                       current_filter.values[2][2] * image -> color_image[line + 1][column + 1].green;

                    result -> color_image[line][column].blue =
                       current_filter.values[0][0] * image -> color_image[line - 1][column - 1].blue +
                       current_filter.values[0][1] * image -> color_image[line - 1][column].blue +
                       current_filter.values[0][2] * image -> color_image[line - 1][column + 1].blue +
                       current_filter.values[1][0] * image -> color_image[line][column - 1].blue +
                       current_filter.values[1][1] * image -> color_image[line][column].blue +
                       current_filter.values[1][2] * image -> color_image[line][column + 1].blue +
                       current_filter.values[2][0] * image -> color_image[line + 1][column - 1].blue +
                       current_filter.values[2][1] * image -> color_image[line + 1][column].blue +
                       current_filter.values[2][2] * image -> color_image[line + 1][column + 1].blue;

                    image -> color_image[line][column] = result -> color_image[line][column];
                }   
            }
         }
    }

    free(result);
}


int main(int argc, char *argv[])
{   
    /**
     * Initialize variables for MPI API
     **/ 
    int rank;
    int number_of_processes;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);

    if (rank == MASTER)
    {
      /**
       *  Master process; it handles most of the work during the execution of the program
       *  TODO: add program execution scheme here and in Readme 
       *  
       **/   

       if (argc < 4)
       {
           printf("The process needs at least 3 parameters to be executed: \n\t input image - output image - filter / filters \n");
           exit(1);
       }
       
       int start_line;
       int end_line;
       char *input_file_name = argv[1];
       char *output_file_name = argv[2];
       
       Image *input_image = read_image(input_file_name);
       
       for (int slave = 1; slave < number_of_processes; ++slave)
       {
           start_line = (slave * input_image -> height) / number_of_processes;
           end_line = ((slave + 1) * input_image -> height) / number_of_processes;

           /**
            *  Wrap the whole image that is send with another line top and buttom
            **/  
           start_line--;
           if (slave != number_of_processes - 1)
           {
               end_line++;
           }

           send_image(input_image, slave, start_line, end_line);
       }

       /**
        *  The master process is also used to apply filters on a image if 
        *  if is the single process and even there are a buch of slaves 
        *  he will perform filtering the image to save time.
        * 
        **/ 

        int start_line_master = 0;
        int end_line_master = input_image -> height / number_of_processes;

        if (end_line != input_image -> height)
        {
            end_line++;
        }

        for (int filter_index = 3; filter_index < argc; ++filter_index)
        {
            apply_filter(input_image, get_filter_by_name(argv[filter_index]), start_line, end_line);
        }

        /**
         * TODO: verify if there is a need in sending the last computed line to the next process and 
         *      receive the last computed line from the previus process
         *  Se mai aduaga ceva in plus si in slave
         **/

        for (int slave = 1; slave < number_of_processes; ++slave)
        {
            start_line = (slave * input_image -> height) / number_of_processes;
            end_line = ((slave + 1) * input_image -> height) / number_of_processes;
            
            Image *received_image  = receive_image(slave);
            for (int line = start_line; line < end_line; ++line)
            {
                if (input_image -> type == PGM)
                {
                    input_image -> image[line] = received_image -> image[line];
                }
                
            }
        }

        write_image(input_image, output_file_name);        

    }
    else
    {
        /**
         *  Slave process
         **/ 
        
        Image *slave_image = receive_image(MASTER);

        for (int filter_index = 3; filter_index < argc; ++filter_index)
        {
            apply_filter(slave_image, get_filter_by_name(argv[filter_index]), 0, slave_image -> height);
        }
        
    }
    
    
    MPI_Finalize();
    return 0;
}
