/* Creates a classifier model using the hierarchical clustering method
 * training and testing files should be in csv format with containing only data points
 * each point consists of N floating point numbers corresponding to its N-dimentional X coordinates
 * the last column is an unsigned integer corresponding to its label
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


typedef struct point
{
    double * x;
    int dimension;
} Point;

Point point_create( int dimension )
{
    Point p;
    p.dimension = dimension;
    p.x = calloc( dimension, sizeof( double ) );
    return p;
}

Point point_copy( Point p )
{
    Point c = point_create( p.dimension );
    int i;
    for( i=0; i<c.dimension; i++ )
    {
        c.x[i] = p.x[i];
    }
    return c;
}

void point_free( Point p )
{
    free( p.x );
}

void point_print( Point p )
{
    int i;
    for( i=0;i<p.dimension;i++ )
    {
        printf( " %.2f",p.x[i] );
    }
}

typedef struct dataset
{
    Point * point;
    int size;
    int * label;
    int number_of_labels;
} Dataset;

Dataset dataset_initialize( int size, int dimension )
{
    Dataset d;
    d.size = size;
    d.label = malloc( size*sizeof( int ) );
    d.point = malloc( size*sizeof( Point ) );
    int i;
    for( i=0; i<size; i++ )
    {
        d.point[i] = point_create( dimension );
    }
    return d;
}

void dataset_free( Dataset d )
{
    int i;
    for( i=0; i<d.size; i++ )
    {
        point_free( d.point[i] );
    }
    free( d.label );
}

int count_lines_of_file( FILE * file )
{
    long original_position = ftell( file );
    fseek( file,0,SEEK_SET );
    int lines = 0;
    while( !feof( file ) )
    {
        if( fgetc( file )=='\n' )lines++;
    }
    fseek( file,original_position,SEEK_SET );
    return lines;
}

/* Counts how many columns a csv file has.
 * Assumes that all lines have the same amount of columns.
 */
int count_columns_of_csv_file( FILE * file )
{
    long original_position = ftell( file );
    fseek( file,0,SEEK_SET );
    int columns = 1;
    char c=0;
    while( !feof( file ) && c!='\n' )
    {
        c = getc( file );
        if( c==',' )columns++;
    }
    fseek( file,original_position,SEEK_SET );
    return columns;
}

void load_points_from_file_into_dataset( FILE * file, Dataset d )
{
    int i,j;
    for( i=0; i<d.size; i++ )
    {
        for( j=0; j<d.point[i].dimension; j++ )
        {
            int ret = fscanf( file, "%lf,",&d.point[i].x[j] );
            //Pre processing hack
            //if(d.point[i].x[j] < 0) d.point[i].x[j]*=-1;
            if( ret != 1 )
            {
                printf( "Failed to read file, wrong x format\n" );
            }
        }
        int ret = fscanf( file, "%d\n", &d.label[i] );
        if( ret != 1 )
        {
            printf( "Failed to read file, wrong label format\n" );
        }
    }
}

int dataset_find_max_label( Dataset d )
{
    int i;
    int max_label = 0;
    for( i=0; i<d.size; i++ )
    {
        if( d.label[i] > max_label )max_label = d.label[i];
    }
    return max_label+1;
}

/* This function reads the contents of a file in csv format.
 * The columns of this file are the coordinates of X.
 * The last column is the label
 */
Dataset read_dataset_from_file( char * file_path )
{
    FILE * file = fopen( file_path, "r" );
    if( file == NULL )
    {
        printf( "Could not open file %s\n",file_path );
        exit( 1 );
    }

    int lines = count_lines_of_file( file );
    int columns = count_columns_of_csv_file( file );

    Dataset d = dataset_initialize( lines, columns-1 );
    load_points_from_file_into_dataset( file,d );
    d.number_of_labels = dataset_find_max_label(d);

    fclose( file );
    return d;
}


void dataset_print( Dataset d )
{
    int i,j;
    for( i=0;i<d.size;i++ )
    {
        printf( "Point %4d: ", i );
        point_print( d.point[i] );
        printf( " Label = %d\n",d.label[i] );
    }
}

typedef struct hierarchical_clustering_data
{
    Point * point;
    int size;
    int * weight;
} Hierarchical_clustering_data;

Hierarchical_clustering_data * hierarchical_clustering_data_load_from_dataset( Dataset d )
{
    Hierarchical_clustering_data * hc = malloc( sizeof( Hierarchical_clustering_data ) );

    hc->size = d.size;
    hc->weight = malloc( hc->size * sizeof( int ) );
    hc->point  = malloc( hc->size * sizeof( Point ) );

    int i;
    for( i=0; i<hc->size; i++ )
    {
        hc->point[i] = point_copy( d.point[i] );
        hc->weight[i] = 1;
    }

    return hc;
}

void hierarchical_clustering_data_free( Hierarchical_clustering_data * hc )
{
    int i;
    for( i=0; i<hc->size; i++ )
    {
        point_free(hc->point[i]);
    }
    free(hc->weight);
    free(hc->point);
    free(hc);
}
Hierarchical_clustering_data * hierarchical_clustering_data_copy( Hierarchical_clustering_data * hc )
{
    Hierarchical_clustering_data * c = malloc( sizeof( Hierarchical_clustering_data ) );
    c->size = hc->size;
    c->weight = malloc( c->size * sizeof( int ) );
    c->point  = malloc( c->size * sizeof( Point ) );

    int i;
    for( i=0; i<hc->size; i++ )
    {
        c->point[i] = point_copy( hc->point[i] );
        c->weight[i] = hc->weight[i];
    }

    return c;
}
void hierarchical_clustering_data_print( Hierarchical_clustering_data * hc )
{
    int i,j;
    for( i=0;i<hc->size;i++ )
    {
        printf( "Point %4d: ", i );
        point_print( hc->point[i] );
        printf( " weight = %d\n",hc->weight[i] );
    }
}

double point_euclidean_distance_squared( Point a, Point b )
{
    int i;
    double distance = 0;
    double delta;

    for( i=0; i<a.dimension; i++ )
    {
        delta = a.x[i] - b.x[i];
        distance += delta * delta;
    }

    return distance;
}

void find_index_of_closest_points( Point * point, int array_size, int * index_a, int * index_b )
{
    int i, j;
    double minimum_distance = INFINITY;

    for( i=0; i<array_size; i++ )
    {
        for( j=i+1; j<array_size; j++ )
        {
            double distance = point_euclidean_distance_squared( point[i], point[j] );
            if( distance < minimum_distance )
            {
                minimum_distance = distance;
                *index_a = i;
                *index_b = j;
            }
        }
    }
}

void calculate_cluster_center_and_store_it_in_a( Hierarchical_clustering_data * hc, int index_a, int index_b )
{
    Point a = hc->point[index_a];
    Point b = hc->point[index_b];

    int w_a = hc->weight[index_a];
    int w_b = hc->weight[index_b];

    int w_sum = w_a + w_b;

    int i;
    for( i=0; i<a.dimension; i++ )
    {
        a.x[i] = ( w_a * a.x[i] + w_b * b.x[i] ) / w_sum;
    }

    hc->weight[index_a] = w_sum;
}
void delete_cluster( Hierarchical_clustering_data * hc, int index )
{
    int last = hc->size - 1;

    point_free( hc->point[index] );

    hc->point[index] = hc->point[last];
    hc->weight[index] = hc->weight[last];

    hc->size--;
}
void reduce_one_cluster( Hierarchical_clustering_data * hc )
{
    int index_a;
    int index_b;
    find_index_of_closest_points( hc->point, hc->size, &index_a, &index_b );
    calculate_cluster_center_and_store_it_in_a( hc, index_a, index_b );
    delete_cluster( hc, index_b );
}
void reduce_to_n_clusters( Hierarchical_clustering_data * hc , int n )
{
    while( hc->size > n )
    {
        printf( "Reducing from %d to %d clusters\n",hc->size, hc->size-1 );
        reduce_one_cluster( hc );
    }
}

typedef struct model
{
    Hierarchical_clustering_data * hc;
    int * translation_table;
} Model;

int find_index_of_closest_cluster( Hierarchical_clustering_data * hc, Point p )
{
    int i, j;
    double minimum_distance = INFINITY;
    int index = -1;

    for( i=0; i<hc->size; i++ )
    {
        double distance = point_euclidean_distance_squared( hc->point[i], p );
        if( distance < minimum_distance )
        {
            minimum_distance = distance;
            index = i;
        }
    }
    return index;
}
//TODO: tidy it up
int * generate_translation_table( Hierarchical_clustering_data * hc, Dataset d )
{
    int * translation_table = calloc( hc->size , sizeof( int ) );
    int ** label_frequency = calloc( hc->size , sizeof( int* ) );
    int i,j;
    for( i=0; i<hc->size; i++ )
    {
        label_frequency[i] = calloc( d.number_of_labels , sizeof( int ) );
    }

    for( i=0; i<d.size; i++ )
    {
        int closest = find_index_of_closest_cluster(hc, d.point[i]);
        label_frequency[find_index_of_closest_cluster(hc, d.point[i])][d.label[i]]++;
    }

    int most_frequent = 0;;
    for( i=0; i<hc->size; i++ )
    {
        for( j=0; j<d.number_of_labels; j++ )
        {
            //printf("[%d][%d] = %d\n",i,j,label_frequency[i][j]);
            if(label_frequency[i][j] > label_frequency[i][most_frequent])
            {
                most_frequent = j;
            }
        }
        translation_table[i] = most_frequent;
    }

    for( i=0; i<hc->size; i++ )
    {
        free(label_frequency[i]);
    }
    free(label_frequency);

    return translation_table;
}

Model * model_load( Hierarchical_clustering_data * hc, Dataset d )
{
    Model * m = malloc( sizeof( Model ) );
    m->hc = hierarchical_clustering_data_copy( hc );
    m->translation_table = generate_translation_table( hc, d );
    return m;
}

void model_free( Model * m )
{
    hierarchical_clustering_data_free( m->hc );
    free( m->translation_table );
    free( m );
}

int model_evaluate( Model * m, Point p )
{
    int cluster = find_index_of_closest_cluster( m->hc, p );
    return m->translation_table[cluster];
}

void model_print( Model * m )
{
    int i;
    printf("Translation: \n");
    for( i=0; i<m->hc->size; i++ )
    {
        printf(" %d -> %d\n",i,m->translation_table[i]);
    }
    hierarchical_clustering_data_print( m->hc );
}

double model_test( Model * m, Dataset t )
{
    int i;
    int hit = 0;
    int miss = 0;
    int * vhit = calloc( t.number_of_labels , sizeof( int ) );
    int * vmiss = calloc( t.number_of_labels , sizeof( int ) );
    for( i=0; i<t.size; i++ )
    {
        int target = t.label[i];
        int result = model_evaluate( m, t.point[i] );
        //printf("Should be %d, got %d\n",target, result);
        if( result == target )
        {
            hit++;
            vhit[t.label[i]]++;
        }
        else
        {
            miss++;
            vmiss[t.label[i]]++;
        }
    }
    double average_label_accuracy = 0;
    for( i=0; i<t.number_of_labels; i++)
    {
        printf("label %d: accuracy = %.2lf%%, hit = %d, miss = %d\n",i,100*(double)vhit[i]/(vhit[i]+vmiss[i]),vhit[i],vmiss[i]);
        average_label_accuracy+=(double)vhit[i]/(vhit[i]+vmiss[i]);
    }
    average_label_accuracy /= t.number_of_labels;
    printf("general accuracy = %.2lf, hit = %d, miss = %d\n",100*(double)hit/(hit+miss),hit,miss);
    printf("average label accuracy = %.2lf\n",100*average_label_accuracy);
    return (double)hit/(hit+miss);
}

int main()
{
    char training_filename[1000];
    printf("Enter name of training file: ");
    scanf("%s", training_filename);
    Dataset d = read_dataset_from_file( training_filename );
    printf( "Dataset: \n" );
    dataset_print( d );
    Hierarchical_clustering_data * hc = hierarchical_clustering_data_load_from_dataset( d );
    //printf( "HC data: \n" );
    //hierarchical_clustering_data_print( hc );
    //printf("Enter desired ammount of clusters: ");
    //int ammount_of_clusters;
    //reduce_to_n_clusters( hc, ammount_of_clusters );
    //printf( "HC data: \n" );
    //hierarchical_clustering_data_print( hc );
    char testing_filename[1000];
    printf("Enter name of testing file: ");
    scanf("%s", testing_filename);
    Dataset t = read_dataset_from_file( testing_filename );
    double max = 0;
    int i;
    printf("Finding ideal ammount of clusters...\n");
    for( i=100; i>=1; i-- )
    {
        printf("%d clusters:\n",i);
        reduce_to_n_clusters( hc, i );
        Model * m = model_load( hc, d );
        //model_print(m);
        double acc = model_test(m, t);
        if(max <= acc)
        {
            max = acc;
            printf("MAX\n");
        }

        model_free(m);
        printf("\n");
    }
    return 0;
}
