#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#include <math.h>


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
        { -1.0, -1.0, -1.0}
    }
};

const filter emboss =
{
    "emboss",
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
    if (strcmp(filter_name, "emboss") == 0) return emboss;

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
 *  multiplication by clamping values to 0 for negative ones and to 255 
 *  for unsigned char values that goes above the max value.
 * 
 *  Basically I allocate a copy of the image that I receive from the input and 
 *  then I modify it according to the rule explained poorly in the PDF :)
 * 
 * 
 **/
Image *apply_filter(Image *img, filter current_filter, int start_line, int end_line) {
  Image *result = malloc(sizeof(Image));
  
  unsigned char **r = (unsigned char **) malloc(img -> height * sizeof(unsigned char *));
  for (int i = 0; i < img -> height; i++)
  {
      r[i] = (unsigned char *) malloc(img -> width * sizeof(unsigned char));
  }

  pixel **r_color = (pixel **) malloc(img -> height * sizeof(pixel *));
  for (int i = 0; i < img -> height; i++)
  {
      r_color[i] = (pixel *) malloc(img -> width * sizeof(pixel));
  }
  
  if (img -> type == PGM) {
        for ( int i = 0; i < img -> height; i++)
        {
            for (int j = 0; j < img -> width; j++)
            {
                r[i][j] = img -> image[i][j];
            }
            
        }
    result -> image = r;
  }
  else
  {
       for ( int i = 0; i < img -> height; i++)
        {
            for (int j = 0; j < img -> width; j++)
            {
                r_color[i][j] = img -> color_image[i][j];
            }
        
        }
    result -> color_image = r_color;
  }
  
  result -> width = img -> width;
  result -> height = img -> height;
  result -> max_val = img -> max_val;
  result -> type = img -> type;

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
                       if (i + offset_i < 0 || i + offset_i == img -> height ||
                           j + offset_j < 0 || j + offset_j == img -> width)
                        {
                            result_pixel_value += 0.0;
                        }
                       else
                        {
                            float image_pixel = (float) img -> image[i + offset_i][j + offset_j];
                            float filter_associated_value = current_filter.values[1 - offset_i][1 - offset_j];
                            result_pixel_value += (filter_associated_value * image_pixel);  
                        }
                   }
                }

                if (result_pixel_value > 255)  result_pixel_value = 255;
                if (result_pixel_value < 0)  result_pixel_value = 0;
               
                result -> image[i][j] = (unsigned char) result_pixel_value;
            }   
        }

        return result;
  } 
  else 
  {
      for (int i = start_line; i < end_line; i++) 
        {
            for (int j = 0; j < img -> width; j++)
            {
               float result_pixel_red = 0.0;
               float result_pixel_green = 0.0;
               float result_pixel_blue = 0.0;
               
               for (int offset_i = -1; offset_i <= 1; offset_i++)
               {
                   for(int offset_j = -1; offset_j <= 1; offset_j++)
                   {
                       if (i + offset_i == -1 || i + offset_i == img -> height ||
                           j + offset_j == -1 || j + offset_j == img -> width)
                        {
                            result_pixel_red += 0.0;
                            result_pixel_green += 0.0;
                            result_pixel_blue += 0.0;
                        }
                       else
                        {   
                            pixel p = img -> color_image[i + offset_i][j + offset_j];
                            float image_pixel_red = (float) p.red;
                            float image_pixel_green = (float) p.green;
                            float image_pixel_blue = (float) p.blue;

                            float filter_associated_value = current_filter.values[1 - offset_i][1 - offset_j];
                            result_pixel_red += (image_pixel_red * filter_associated_value);
                            result_pixel_green += (image_pixel_green * filter_associated_value);
                            result_pixel_blue += (image_pixel_blue * filter_associated_value);

                        }
                   }
                }
                if (result_pixel_red > 255) result_pixel_red = 255;
                if (result_pixel_red < 0) result_pixel_red = 0;
                if (result_pixel_green > 255) result_pixel_green = 255;
                if (result_pixel_green < 0) result_pixel_green = 0;
                if (result_pixel_blue > 255) result_pixel_blue = 255;
                if (result_pixel_blue < 0) result_pixel_blue = 0;       
                pixel result_pixel = 
                {
                    (unsigned char) result_pixel_red,
                    (unsigned char) result_pixel_green,
                    (unsigned char) result_pixel_blue
                };
                result->color_image[i][j] = result_pixel;
           
            }   
        }
        
        return result;
  }
}


/**
 * Main entry of the process that handles the image distribution 
 * and the data gathering from all the slave processes.
 * 
 * 
 **/ 
int main(int argc, char *argv[]) {

  int rank;
  int number_of_processes;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);

  if (rank == MASTER) 
  {
    int start_line;
    int end_line;

    if (argc < 4) 
    {
      printf("\n\t Please provide at least 3 arguments for the executable: \n\t mpirun -np P ./executable image_in image_out filter_1 filter_2 ...\n Although the executable stop working the lamboot background daemon will continue to work \n\t please end it with Ctrl + C\n");
      exit(-1);
    }

    Image *image = read_image(argv[1]);
    Image *out = read_image(argv[1]);
    
    for (int i = 3; i < argc; i++) 
    {
       
        int mul_factor = (int)ceil((1.0 * image -> height) / number_of_processes);
        int low_bound = mul_factor * rank;
        int high_bound= (int)fmin(mul_factor * (rank + 1), image -> height);
        
        int start_line_master = low_bound;
        int end_line_master = high_bound;

        if (number_of_processes == 1)
        {
            end_line_master = image -> height;
        }
        
         /**
         *  Copy the content of the current image received from read_image funstion
         *  into the out image receiver buffer wich will be the working one during 
         *  filtering process.
         * 
         **/ 
        if (image -> type == PGM)
        {
            for ( int i = 0; i < image -> height; i++)
            {
                for (int j = 0; j < image -> width; j++)
                {
                    image -> image[i][j] = out -> image[i][j];
                }
            
            }
        }
        else
        {
            for ( int i = 0; i < image -> height; i++)
            {
                for (int j = 0; j < image -> width; j++)
                {
                    image -> color_image[i][j] = out -> color_image[i][j];
                }
            
            }
        }
        

        for (int i = 1; i < number_of_processes; i++) 
        {
            start_line = 0;
            end_line = image->height;
            send_image(image, i, start_line, end_line);
        }

        out = apply_filter(image, get_filter_by_name(argv[i]), start_line_master, end_line_master);
    
        for (int i = 1; i < number_of_processes; i++) 
        {
            int mul_factor_i = (int)ceil((1.0 * image -> height) / number_of_processes);
            int low_bound_i = mul_factor * i;
            int high_bound_i = (int)fmin(mul_factor * (i + 1), image -> height);

            start_line = low_bound_i;
            end_line = high_bound_i;
            
            Image *img = receive_image(i);

            for (int j = start_line; j < end_line; j++) 
            {
                if (img -> type == PGM)
                {
                    out->image[j] = img->image[j];
                }
                else
                {
                    out->color_image[j] = img->color_image[j];
                }
            }
        }
    }

    write_image(out, argv[2]);
   
  } else {
    
    Image *img;
    Image *out_slave;
    
    for (int i = 3; i < argc; i++) 
    {
      img = receive_image(MASTER);
      
      int mul_factor = (int)ceil((1.0 * img -> height) / number_of_processes);
      int low_bound = mul_factor * rank;
      int high_bound= (int)fmin(mul_factor * (rank + 1), img -> height);

      int start_line_slave = low_bound;
      int end_line_slave = high_bound;
      
      if (rank == number_of_processes - 1)
      {
          end_line_slave = img -> height;
      }

      out_slave = apply_filter(img, get_filter_by_name(argv[i]), start_line_slave, end_line_slave);
      send_image(out_slave, 0, 0, img -> height);
    }
  }
  
  MPI_Finalize();
  return 0;
}