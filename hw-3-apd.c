#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>



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
    printf("Send\n");
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
    printf("Received\n");
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
Image *apply_filter(Image *img, filter current_filter, int start_line, int end_line) {
  Image *result = img;
  
  if (img -> type == PGM)
  {
        for (int i = start_line; i < end_line; i++) 
        {
            for (int j = 0; j < img -> width; j++)
            {
               float result_pixel_value = 0.0;

               for (int offset_i = -1; offset_i <= 1; offset_i++)
               {
                   for(int offset_j = -1; offset_j <= 1; offset_j++)
                   {
                       if (i + offset_i == -1 || i + offset_i == img -> height ||
                           j + offset_j == -1 || j + offset_j == img -> width)
                        {
                            result_pixel_value += 0.0;
                        }
                       else
                        {
                            float image_pixel = (float) img -> image[i + offset_i][j + offset_j];
                            float filter_associated_value = current_filter.values[1 - offset_i][1 - offset_j];
                            result_pixel_value += (image_pixel * filter_associated_value);  
                        }
                   }
               }

                if (result_pixel_value > 255) 
                    result_pixel_value = 255;
                if (result_pixel_value < 0) 
                    result_pixel_value = 0;
                    
                printf("Sum: %f   ", result_pixel_value);
                result -> image[i][j] = (unsigned char) result_pixel_value;
                printf("Before: %u:   After: %u \n", img -> image[i][j], result -> image[i][j]);
            
            }   
        }

        // for (int i = 0; i < img -> height; i++)
        // {
        //     for (int j = 0; j < img -> width; j++)
        //     {
        //         img -> image[i][j] = result -> image[i][j];
        //     }
        // }

        return result;
  } 
  else 
  {
      return NULL;
  }

 return NULL; 
}



int main(int argc, char *argv[]) {

  int rank;
  int number_of_processes;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);

  if (rank == MASTER) {
    
    int start_line;
    int end_line;

    if (argc < 4) {
      printf("\n\t Please provide at least 3 arguments for the executable: \n\t mpirun -np P ./executable image_in image_out filter_1 filter_2 ...\n Although the executable stop working the lamboot background daemon will continue to work \n\t please end it with Ctrl + C\n");
      exit(-1);
    }

    Image *image = malloc(sizeof(Image));
    image = read_image(argv[1]);
    
    for (int i = 1; i < number_of_processes; i++) 
    {
      start_line = (i * image->height) / number_of_processes;
      end_line = ((i + 1) * image->height) / number_of_processes;

      start_line -= 1;
      if (i != number_of_processes - 1)
        end_line++;

      send_image(image, i, start_line, end_line);
    }

    start_line = 0;
    end_line = image->height / number_of_processes;

    if (end_line != image->height)
      end_line++;

    for (int i = 3; i < argc; i++) 
    {
      image = apply_filter(image, get_filter_by_name(argv[i]), start_line, end_line);
    }


    for (int i = 1; i < number_of_processes; i++) 
    {

      start_line = (i * image->height) / number_of_processes;
      end_line = ((i + 1) * image->height) / number_of_processes;
      
      Image *img = receive_image(i);
      free(img->image[0]);

      for (int j = start_line; j < end_line; j++) 
      {

        free(image->image[j]);

        image->image[j] = img->image[j - start_line + 1];
      }

      for (int j = end_line - start_line + 2; j < img->height; j++)
        free(img->image[j]);

      free(img->image);
      free(img);
    }

    write_image(image, argv[2]);

  } else {
    
    Image *img = receive_image(0);

    for (int i = 3; i < argc; i++) 
    {
      img = apply_filter(img, get_filter_by_name(argv[i]), 0, img->height);
    }
    send_image(img, 0, 0, img->height);
  }

  MPI_Finalize();
  return 0;
}