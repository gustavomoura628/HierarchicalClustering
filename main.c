#include <stdio.h>
#include <stdlib.h>

typedef struct point
{
    double * x;
    int dimension;
} Point;

Point point_create(int dimension)
{
    Point p;
    p.dimension = dimension;
    p.x = calloc(dimension, sizeof(double));
    return p;
}

void point_free(Point p)
{
    free(p.x);
}

typedef struct dataset
{
    Point * point;
    int size;
    int * label;
} Dataset;

Dataset dataset_initialize(int size, int dimension)
{
    Dataset d;
    d.size = size;
    d.label = malloc(size*sizeof(int));
    d.point = malloc(size*sizeof(Point));
    int i;
    for( i=0; i<size; i++)
    {
        d.point[i] = point_create(dimension);
    }
    return d;
}

void dataset_free(Dataset d)
{
    int i;
    for( i=0; i<d.size; i++)
    {
        point_free(d.point[i]);
    }
    free(d.label);
}

int count_lines_of_file(FILE * file)
{
    long original_position = ftell(file);
    fseek(file,0,SEEK_SET);
    int lines = 0;
    while(!feof(file))
    {
        if(fgetc(file)=='\n')lines++;
    }
    fseek(file,original_position,SEEK_SET);
    return lines;
}

/* Counts how many columns a csv file has.
 * Assumes that all lines have the same amount of columns.
 */
int count_columns_of_csv_file(FILE * file)
{
    long original_position = ftell(file);
    fseek(file,0,SEEK_SET);
    int columns = 1;
    char c=0;
    while(!feof(file) && c!='\n')
    {
        c = getc(file);
        if(c==',')columns++;
    }
    fseek(file,original_position,SEEK_SET);
    return columns;
}

void load_points_from_file_into_dataset(FILE * file, Dataset d)
{
    int i,j;
    for( i=0; i<d.size; i++)
    {
        for( j=0; j<d.point[i].dimension; j++)
        {
            int ret = fscanf(file, "%lf,",&d.point[i].x[j]);
            if(ret != 1)
            {
                printf("Failed to read file, wrong x format\n");
            }
        }
        int ret = fscanf(file, "%d\n", &d.label[i]);
        if(ret != 1)
        {
            printf("Failed to read file, wrong label format\n");
        }
    }
}

/* This function reads the contents of a file in csv format.
 * The columns of this file are the coordinates of X.
 * The last column is the label
 */
Dataset read_dataset_from_file(char * file_path)
{
    FILE * file = fopen(file_path, "r");
    if( file == NULL )
    {
        printf("Could not open file %s\n",file_path);
        exit(1);
    }

    int lines = count_lines_of_file(file);
    int columns = count_columns_of_csv_file(file);

    Dataset d = dataset_initialize(lines, columns-1);
    load_points_from_file_into_dataset(file,d);

    fclose(file);
    return d;
}

void print_dataset(Dataset d)
{
    int i,j;
    for(i=0;i<d.size;i++)
    {
        printf("Point %4d: ", i);
        for(j=0;j<d.point[i].dimension;j++)
        {
            printf("%.2f ",d.point[i].x[j]);
        }
        printf(" Label = %d\n",d.label[i]);
    }
}

int main()
{
    Dataset d = read_dataset_from_file("training.csv");
    print_dataset(d);
    return 0;
}
