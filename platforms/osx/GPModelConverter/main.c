/*
 * This file is part of SceneFlipEngine.
 * Copyright 2012, 2017 Paul Chote
 *
 * SceneFlipEngine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SceneFlipEngine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SceneFlipEngine.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#import <OpenGL/OpenGL.h>

#pragma mark obj -> mdl conversion
struct model_header
{
    uint32_t version;
    uint32_t frame_count;
    uint32_t triangle_count;
    uint32_t texture_name_length;
};

/*
 * Parse an obj file and return the number of vertices, texcoords, and triangles by reference
 */
static void count_obj_entries(FILE *file, size_t *vertex_count, size_t *texcoord_count, size_t *triangle_count)
{
    size_t vi = 0;
    size_t tci = 0;
    size_t ti = 0;
    char linebuf[1024];
    fseek(file, 0, SEEK_SET);
    
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL)
    {
        if (strncmp(linebuf, "v ", 2) == 0)
            vi++;
        
        if (strncmp(linebuf, "vt ", 3) == 0)
            tci++;
        
        if (strncmp(linebuf, "f ", 2) == 0)
        {
            // Split face into numentries - 2 triangles
            char *search = linebuf;
            size_t tokens = 0;
            while (strsep(&search, " \t") != NULL)
                tokens++;
            assert(tokens >= 4);
            ti += tokens - 3; // Ignore first token ('f')
        }
    }
    *vertex_count = vi;
    *texcoord_count = tci;
    *triangle_count = ti;
}

/*
 * Parse an obj file and load vertex definitions
 * Returns the number of vertices loaded
 */
static size_t load_obj_vertices(FILE *file, GLfloat *vertices)
{
    size_t vi = 0;
    char linebuf[1024];
    fseek(file, 0, SEEK_SET);
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL)
    {
        if (strncmp(linebuf, "v ", 2) == 0)
        {
            sscanf(linebuf, "v %f %f %f", &vertices[3*vi], &vertices[3*vi+1], &vertices[3*vi+2]);
            vi++;
        }
    }
    return vi;
}

/*
 * Parse an obj file and load vertices, texcoords, and triangle definitions
 */
static void load_obj_arrays(FILE *file, GLfloat *vertices, GLfloat *texcoords, size_t *triangles,
                            size_t *vertex_count, size_t *texcoord_count, size_t *triangle_count)
{
    size_t vi = 0;
    size_t tci = 0;
    size_t ti = 0;
    char linebuf[1024];
    fseek(file, 0, SEEK_SET);
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL)
    {
        if (strncmp(linebuf, "v ", 2) == 0)
        {
            sscanf(linebuf, "v %f %f %f", &vertices[3*vi], &vertices[3*vi+1], &vertices[3*vi+2]);
            vi++;
        }
        
        if (strncmp(linebuf, "vt ", 3) == 0)
        {
            sscanf(linebuf, "vt %f %f", &texcoords[2*tci], &texcoords[2*tci+1]);
            tci++;
        }
        
        if (strncmp(linebuf, "f ", 2) == 0)
        {
            // read tokens into triangles 
            char *search = &linebuf[2];
            char *token;
            size_t i = 0; // Vertices read
            while ((token = strsep(&search, " \t")) != NULL)
            {
                size_t vi, tci, ni; // vertex,texcoord,normal indices (1-indexed)
                size_t read = sscanf(token, "%zu/%zu/%zu", &vi, &tci, &ni);
                assert(read >= 2);
                
                // Load first triangle from face
                if (i < 3)
                {
                    triangles[6*ti + 2*i] = vi-1;
                    triangles[6*ti + 2*i + 1] = tci-1;
                    i++;
                }
                else
                {
                    // Create new triangle with the first, last, and new vertices
                    size_t *last = &triangles[6*ti];
                    size_t poly[] = {last[0], last[1], last[4], last[5], vi-1, tci-1};
                    ti++;
                    
                    memcpy(&triangles[6*ti], poly, 6*sizeof(size_t));
                    i += 3;
                }
            }
            ti++;
        }
    }
    *vertex_count = vi;
    *texcoord_count = tci;
    *triangle_count = ti;
}

void model_convert_obj(const char **input, size_t input_count, const char *output)
{
    assert(input_count > 0);
    printf("Loading %zu frames...\n", input_count);

    // Define model from first frame
    size_t vertex_count, texcoord_count, triangle_count;
    FILE *first = fopen(input[0], "r");
    assert(first);
    count_obj_entries(first, &vertex_count, &texcoord_count, &triangle_count);
    
    // Load raw data from first frame and ensure that all the entries were valid
    GLfloat *vertices = calloc(3*input_count*vertex_count, sizeof(GLfloat));
    GLfloat *texcoords = calloc(2*texcoord_count, sizeof(GLfloat));
    size_t *triangles = calloc(6*triangle_count, sizeof(size_t));
    size_t loaded_vertex_count, loaded_texcoord_count, loaded_triangle_count;
    load_obj_arrays(first, vertices, texcoords, triangles,
                    &loaded_vertex_count, &loaded_texcoord_count, &loaded_triangle_count);
    fclose(first);
    
    assert(loaded_vertex_count == vertex_count);
    assert(loaded_texcoord_count == texcoord_count);
    assert(loaded_triangle_count == triangle_count);
    
    // Load vertices from subsequent frames (face and texcoord definitions are ignored)
    for (size_t i = 1; i < input_count; i++)
    {
        FILE *frame = fopen(input[i], "r");
        size_t loaded = load_obj_vertices(frame, &vertices[3*i*vertex_count]);
        assert(loaded == vertex_count);
        fclose(frame);
    }
    
    // Build vertex and texcoord data arrays from triangle definitions
    GLfloat *vertex_data = calloc(9*input_count*triangle_count, sizeof(GLfloat));
    GLfloat *texcoord_data = calloc(6*triangle_count, sizeof(GLfloat));
    for (size_t ti = 1; ti < triangle_count; ti++)
        for (size_t j = 0; j < 3; j++)
        {
            size_t vi = triangles[6*ti+2*j];
            size_t tci = triangles[6*ti+2*j+1];
            memcpy(&texcoord_data[6*ti + 2*j], &texcoords[2*tci], 2*sizeof(GLfloat));
            for (size_t fi = 0; fi < input_count; fi++)
                memcpy(&vertex_data[9*(triangle_count*fi + ti) + 3*j], &vertices[3*(vertex_count*fi + vi)], 3*sizeof(GLfloat));
        }
    
    free(texcoords);
    free(vertices);
    free(triangles);
    
    // TODO: Load this from the obj file
    char *texture_name = "knight.png";
    struct model_header h = 
    {
        .version = 1,
        .frame_count = (uint32_t)input_count,
        .triangle_count = (uint32_t)triangle_count,
        .texture_name_length = (uint32_t)strlen(texture_name)
    };
    
    FILE *mdl = fopen(output, "wb");
    assert(mdl);

    fwrite(&h, sizeof(struct model_header), 1, mdl);
    fwrite(vertex_data, sizeof(GLfloat), 9*h.frame_count*h.triangle_count, mdl);
    fwrite(texcoord_data, sizeof(GLfloat), 6*h.triangle_count, mdl);
    fwrite(texture_name, sizeof(char), h.texture_name_length, mdl);
    fclose(mdl);
    printf("Saved to %s\n", output);
    free(vertex_data);
}

typedef struct
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
} vertex;

typedef struct
{
    GLfloat x;
    GLfloat y;
} texcoord;

typedef struct
{
    uint32_t vertex_index[4];
    uint32_t texcoord_index[4];
    bool has_texcoords;
    uint32_t normal_index[4];
    bool has_normals;
    uint32_t num_vertices;
    uint32_t group;
} face;

typedef struct face_list
{
    uint32_t vertex;
    uint32_t group;
    struct face_list *next;
    struct face_list *prev;
} face_list;

uint32_t load_groups(FILE *file, char ***out_groups)
{
    // Count vertices
    char linebuf[1024];
    fseek(file, 0, SEEK_SET);
    uint32_t count = 0;
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL)
    {
        if (strncmp(linebuf, "o ", 2) == 0)
            count++;
    }

    if (count == 0)
    {
        *out_groups = NULL;
        return 0;
    }

    // Load vertices
    char **groups = calloc(count, sizeof(char *));
    assert(groups);
    fseek(file, 0, SEEK_SET);
    uint32_t i = 0;
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL && i < count)
        if (strncmp(linebuf, "o ", 2) == 0)
        {
            groups[i] = strdup(&linebuf[2]);
            // Remove trailing newline
            groups[i][strlen(groups[i])-1] = '\0';
            i++;
        }

    *out_groups = groups;
    return i;
}


uint32_t load_vertices(FILE *file, vertex **out_vertices)
{
    // Count vertices
    char linebuf[1024];
    fseek(file, 0, SEEK_SET);
    uint32_t count = 0;
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL)
    {
        if (strncmp(linebuf, "v ", 2) == 0)
            count++;
    }

    if (count == 0)
    {
        *out_vertices = NULL;
        return 0;
    }

    // Load vertices
    vertex *vertices = calloc(count, sizeof(vertex));
    assert(vertices);
    fseek(file, 0, SEEK_SET);
    uint32_t i = 0;
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL && i < count)
    {
        if (strncmp(linebuf, "v ", 2) == 0)
        {
            // obj/blender uses weird axes
            sscanf(linebuf, "v %f %f %f", &vertices[i].x, &vertices[i].z, &vertices[i].y);
            i++;
        }
    }

    *out_vertices = vertices;
    return count;
}

uint32_t load_faces(FILE *file, face **out_faces)
{
    // Count vertices
    char linebuf[1024];
    fseek(file, 0, SEEK_SET);
    uint32_t count = 0;
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL)
    {
        if (strncmp(linebuf, "f ", 2) == 0)
            count++;
    }

    if (count == 0)
    {
        *out_faces = NULL;
        return 0;
    }

    // Load faces
    face *faces = calloc(count, sizeof(face));
    assert(faces);
    fseek(file, 0, SEEK_SET);
    uint32_t j = 0, group_count = 0;
    bool first_group = true;
    while (fgets(linebuf, sizeof(linebuf)-1, file) != NULL && j < count)
    {
        if (strncmp(linebuf, "o ", 2) == 0)
        {
            if (first_group)
                first_group = false;
            else
                group_count++;
        }

        if (strncmp(linebuf, "f ", 2) == 0)
        {
            // read tokens into triangles
            char *search = &linebuf[2];
            char *token;
            uint32_t i = 0; // entries read
            while ((token = strsep(&search, " \t")) != NULL)
            {
                uint32_t vi = 1, tci = 1, ni = 1; // vertex,texcoord,normal indices (1-indexed)
                size_t read = sscanf(token, "%u/%u/%u", &vi, &tci, &ni);
                assert(read > 0);
                assert(i < 4);

                faces[j].vertex_index[i] = vi - 1;
                faces[j].texcoord_index[i] = tci - 1;
                faces[j].has_texcoords = (read > 1);
                faces[j].normal_index[i] = ni - 1;
                faces[j].has_normals = (read > 2);
                faces[j].group = group_count;
                i++;
            }

            faces[j].num_vertices = i;
            j++;
        }
    }

    // Remove lines with only two vertices
    for (size_t j = 0; j < count; j++)
    {
        if (faces[j].num_vertices < 3)
        {
            printf("Discarding face with < 3 vertices\n");
            for (size_t i = j + 1; i < count; i++)
                faces[i-1] = faces[i];

            count--;
            j--;
        }
    }

    // Split quads into triangles
    size_t num_quads = 0;
    for (size_t j = 0; j < count; j++)
        if (faces[j].num_vertices == 4)
            num_quads++;

    if (num_quads > 0)
    {
        faces = realloc(faces, (count + num_quads)*sizeof(face));
        assert(faces);
        for (size_t j = 0; j < count; j++)
            if (faces[j].num_vertices == 4)
            {
                printf("Splitting face with 4 vertices\n");
                size_t k[3] = {0,2,3};
                for (size_t i = 0; i < 3; i++)
                {
                    faces[count].vertex_index[i] = faces[j].vertex_index[k[i]];
                    faces[count].texcoord_index[i] = faces[j].texcoord_index[k[i]];
                    faces[count].has_texcoords = faces[j].has_texcoords;
                    faces[count].normal_index[i] = faces[j].normal_index[k[i]];
                    faces[count].has_normals = faces[j].has_normals;
                    faces[count].num_vertices = 3;
                }
                count++;
                faces[j].num_vertices = 3;
            }
    }
    *out_faces = faces;
    return count;
}

uint32_t remove_duplicate_vertices(vertex *vertices, uint32_t vertex_count, face *faces, uint32_t face_count)
{
    const double threshold = 1e-6;
    for (uint32_t j = 0; j < vertex_count; j++)
        for (uint32_t i = j + 1; i < vertex_count; i++)
        {
            double dx = vertices[j].x - vertices[i].x;
            double dy = vertices[j].y - vertices[i].y;
            double dz = vertices[j].z - vertices[i].z;
            double diff = sqrt(dx*dx + dy*dy + dz*dz);
            //printf("Difference: %f\n", diff);
            if (diff < threshold)
            {
                // Shift all later vertices down an index
                for (uint32_t k = i + 1; k < vertex_count; k++)
                    vertices[k-1] = vertices[k];

                vertex_count--;

                // Fix face references
                for (uint32_t k = 0; k < face_count; k++)
                    for (uint32_t l = 0; l < faces[k].num_vertices; l++)
                    {
                        if (faces[k].vertex_index[l] > i)
                            faces[k].vertex_index[l]--;
                        else if (faces[k].vertex_index[l] == i)
                            faces[k].vertex_index[l] = j;
                    }
                i--;
            }
        }

    return vertex_count;
}

void set_ccw_face_orientation(face *faces, uint32_t face_count, vertex *vertices, uint32_t vertex_count)
{
    for (uint32_t j = 0; j < face_count; j++)
    {
        if (faces[j].num_vertices != 3)
        {
            printf("Face %u of %u with %u vertices!\n", j, face_count, faces[j].num_vertices);
            assert(0);
        }

        // Calculate normal direction
        vertex a = vertices[faces[j].vertex_index[0]];
        vertex b = vertices[faces[j].vertex_index[1]];
        vertex c = vertices[faces[j].vertex_index[2]];
        GLfloat u[2] = {b.x - a.x, b.y - a.y};
        GLfloat v[2] = {c.x - a.x, c.y - a.y};;
        GLfloat n = u[0]*v[1] - u[1]*v[0];

        // Ensure counter-clockwise orientation
        if (n < 0)
        {
            for (size_t i = 0; i < 3; i++)
            {
                uint32_t temp = faces[j].vertex_index[2];
                faces[j].vertex_index[2] = faces[j].vertex_index[0];
                faces[j].vertex_index[0] = temp;
            }
        }
    }
}

bool iterate_face_merging(face_list **border_faces, uint32_t face_count)
{
    for (size_t j = 0; j < face_count; j++) // First polygon in list
    {
        if (border_faces[j] == NULL)
            continue;

        for (size_t i = j + 1; i < face_count; i++) // second polygon in list
        {
            if (border_faces[i] == NULL)
                continue;

            if (border_faces[i]->group != border_faces[j]->group)
                continue;

            // Now, iterate through both polygons, checking if any edges match
            face_list *first = border_faces[j];
            face_list *second = border_faces[i];
            do
            {
                do
                {
                    // Stitch polygons together
                    if (first->vertex == second->vertex && first->next->vertex == second->prev->vertex)
                    {
                        face_list *orig_first_next = first->next;
                        face_list *orig_second = second;
                        face_list *orig_second_prev = second->prev;

                        first->next = second->next;
                        first->next->prev = first;

                        orig_second_prev->prev->next = orig_first_next;
                        orig_first_next->prev = orig_second_prev->prev;

                        free(orig_second);
                        free(orig_second_prev);
                        border_faces[i] = NULL;

                        return true;
                    }
                    second = second->next;
                } while (second != border_faces[i]);

                first = first->next;
            } while (first != border_faces[j]);
        }
    }
    return false;
}

bool iterate_face_splitting(face_list **border_faces, uint32_t face_count)
{
    for (size_t j = 0; j < face_count; j++) // First polygon in list
    {
        if (border_faces[j] == NULL)
            continue;

        face_list *first = border_faces[j];
        face_list *second = border_faces[j]->next;
        do
        {
            do
            {
                // Iterate polygon, looking for lines connecting the same two vertices in opposite directions
                if (first->vertex == second->next->vertex && first->next->vertex == second->vertex)
                {
                    // Cut these edges to form two new polygons
                    face_list *orig_second_next = second->next;
                    face_list *orig_first_next = first->next;

                    second->next = orig_first_next->next;
                    second->next->prev = second;

                    first->next = orig_second_next->next;
                    first->next->prev = first;

                    free(orig_second_next);
                    free(orig_first_next);

                    // Insert first into existing spot in list
                    border_faces[j] = first;

                    // Insert second into the first blank spot in list
                    for (size_t k = 0; k < face_count; k++)
                    {
                        if (border_faces[k] == NULL)
                        {
                            border_faces[k] = second;
                            break;
                        }
                        assert(k != face_count - 1);
                    }

                    return true;
                }
                second = second->next;
            } while (second != border_faces[j]);

            first = first->next;
        } while (first != border_faces[j]);
    }
    return false;
}

bool iterate_border_splitting(face_list **border_faces, uint16_t group_masks[16], uint32_t face_count)
{
    for (uint32_t j = 0; j < face_count; j++) // First polygon in list
    {
        if (border_faces[j] == NULL)
            continue;

        for (uint32_t i = 0; i < face_count; i++)
        {
            if (border_faces[i] == NULL)
                continue;

            // Non-interacting collision groups
            if (!(group_masks[border_faces[j]->group] & (1 << border_faces[i]->group)))
                continue;

            face_list *first = border_faces[j];
            face_list *second = border_faces[i];
            do
            {
                if (first->next == NULL)
                    break;

                do
                {
                    if (second->next == NULL)
                        break;

                    // Iterate polygon, looking for lines connecting the same two vertices in opposite directions
                    if (first->vertex == second->next->vertex && first->next->vertex == second->vertex)
                    {
                        // Cut these edges to form two new polygons
                        face_list *orig_second_next = second->next;
                        face_list *orig_first_next = first->next;

                        // Break linkage to form new line segment
                        first->next->prev = NULL;
                        first->next = NULL;
                        second->next->prev = NULL;
                        second->next = NULL;

                        // Free isolated vertices
                        if (first->next == NULL && first->prev == NULL)
                        {
                            if (first == border_faces[j])
                                border_faces[j] = NULL;
                            free(first);
                        }

                        if (orig_first_next->next == NULL && orig_first_next->prev == NULL)
                        {
                            if (orig_first_next == border_faces[j])
                                border_faces[j] = NULL;
                            free(orig_first_next);
                        }
                        else
                        {
                            // Check that it's not connected to the start vertex
                            // If so, it becomes the new start
                            face_list *next = orig_first_next;
                            while (next != NULL && next != border_faces[j])
                                next = next->next;

                            // This is just an earlier part of the current segment
                            if (next == border_faces[j])
                                border_faces[j] = orig_first_next;
                            else
                            {
                                // Disconnected segmenet - Insert into first blank spot in list
                                for (uint32_t k = 0; k < face_count; k++)
                                {
                                    if (border_faces[k] == NULL)
                                    {
                                        border_faces[k] = orig_first_next;
                                        break;
                                    }
                                    assert(k != face_count - 1);
                                }
                            }
                        }

                        if (second->next == NULL && second->prev == NULL)
                        {
                            if (second == border_faces[i])
                                border_faces[i] = NULL;
                            free(second);
                        }
                        if (orig_second_next->next == NULL && orig_second_next->prev == NULL)
                        {
                            if (orig_second_next == border_faces[i])
                                border_faces[i] = NULL;
                            free(orig_second_next);
                        }
                        else
                        {
                            // Check that it's not connected to the start vertex
                            // If so, it becomes the new start
                            face_list *next = orig_second_next;
                            while (next != NULL && next != border_faces[i])
                                next = next->next;

                            // This is just an earlier part of the current segment
                            if (next == border_faces[i])
                                border_faces[i] = orig_second_next;
                            else
                            {
                                // Disconnected segmenet - Insert into first blank spot in list
                                for (uint32_t k = 0; k < face_count; k++)
                                {
                                    if (border_faces[k] == NULL)
                                    {
                                        border_faces[k] = orig_second_next;
                                        break;
                                    }
                                    assert(k != face_count - 1);
                                }
                            }
                        }

                        return true;
                    }
                    second = second->next;
                } while (second != border_faces[i]);

                first = first->next;
            } while (first != border_faces[j] && first->next != NULL);
        }
    }
    return false;
}

uint32_t calculate_mesh_borders(face *faces, uint32_t face_count, uint16_t group_masks[16], uint32_t **out_border_lengths, uint32_t **out_border_groups, uint32_t ***out_border_vertex_indices)
{
    // Generate an array of linked lists representing the faces
    face_list **border_faces = calloc(face_count, sizeof(face_list *));
    for (size_t j = 0; j < face_count; j++)
    {
        face_list *a = calloc(1, sizeof(face_list));
        face_list *b = calloc(1, sizeof(face_list));
        face_list *c = calloc(1, sizeof(face_list));

        a->vertex = faces[j].vertex_index[0];
        a->group = faces[j].group;
        a->next = b;
        a->prev = c;
        b->vertex = faces[j].vertex_index[1];
        b->group = faces[j].group;
        b->next = c;
        b->prev = a;
        c->vertex = faces[j].vertex_index[2];
        c->group = faces[j].group;
        c->next = a;
        c->prev = b;
        border_faces[j] = a;
    }

    // Join adjacent faces into a larger polygon
    while (iterate_face_merging(border_faces, face_count));

    // Remove common edges by splitting into smaller polygons
    while (iterate_face_splitting(border_faces, face_count));

    // Remove common edges from adjacent borders
    while (iterate_border_splitting(border_faces, group_masks, face_count));

    // Count number of border segments
    uint32_t border_count = 0;
    for (size_t j = 0; j < face_count; j++)
        if (border_faces[j] != NULL)
            border_count++;

    uint32_t *border_lengths = calloc(border_count, sizeof(uint32_t));
    uint32_t *border_groups = calloc(border_count, sizeof(uint32_t));
    uint32_t **border_vertex_indices = calloc(border_count, sizeof(uint32_t*));
    assert(border_lengths && border_groups && border_vertex_indices);

    for (size_t j = 0, i = 0; j < face_count; j++)
        if (border_faces[j] != NULL)
        {
            border_groups[i] = border_faces[j]->group;

            // Count length of each border
            // First vertex is duplicated to ensure a closed loop
            face_list *start = border_faces[j];
            border_lengths[i] = 1;
            face_list *next = start->next;
            do
            {
                next = next->next;
                border_lengths[i]++;
            } while (next != start && next != NULL);

            // Closed loop
            if (next == start)
                border_lengths[i]++;

            // Copy vertex indices into output array
            border_vertex_indices[i] = calloc(border_lengths[i], sizeof(uint32_t));
            next = start;
            for (size_t k = 0; k < border_lengths[i]; k++)
            {
                border_vertex_indices[i][k] = next->vertex;
                next = next->next;
            }

            i++;
        }

    *out_border_lengths = border_lengths;
    *out_border_groups = border_groups;
    *out_border_vertex_indices = border_vertex_indices;
    return border_count;
}

void walkmap_convert_obj(const char *input_filename, const char *output_filename)
{
    // Define model from first frame
    FILE *input = fopen(input_filename, "r");
    assert(input);

    char **group_names;
    uint32_t group_count = load_groups(input, &group_names);
    assert(group_count < 16);

    // Collision group interaction masks
    uint16_t group_masks[16];
    for (uint8_t i = 0; i < 16; i++)
        group_masks[i] = (1 << i);

    for (uint8_t j = 0; j < group_count; j++)
        for (uint8_t i = j + 1; i < group_count; i++)
        {
            printf("Does `%s' interact with `%s'? [y]/n:", group_names[j], group_names[i]);
            char buffer[32];
            fgets(buffer, 32, stdin);
            if (buffer[0] != 'n')
            {
                group_masks[j] |= 1 << i;
                group_masks[i] |= 1 << j;
            }
        }

    vertex *vertices;
    uint32_t vertex_count = load_vertices(input, &vertices);

    face *faces;
    uint32_t face_count = load_faces(input, &faces);

    vertex_count = remove_duplicate_vertices(vertices, vertex_count, faces, face_count);
    set_ccw_face_orientation(faces, face_count, vertices, vertex_count);

    uint32_t *border_lengths, *border_groups;
    uint32_t **border_vertex_indices;
    uint32_t border_count = calculate_mesh_borders(faces, face_count, group_masks, &border_lengths, &border_groups, &border_vertex_indices);

    FILE *output = fopen(output_filename, "wb");
    assert(output);

    // Write file version
    uint32_t version = 1;
    fwrite(&version, sizeof(uint32_t), 1, output);

    // Number of vertices
    fwrite(&vertex_count, sizeof(uint32_t), 1, output);

    // Number of triangles in each collision group
    fwrite(&face_count, sizeof(uint32_t), 1, output);

    // Number of borders in each collision group
    fwrite(&border_count, sizeof(uint32_t), 1, output);

    // Vertex data
    fwrite(vertices, sizeof(vertex), vertex_count, output);

    // Triangle data
    for (size_t i = 0; i < face_count; i++)
    {
        fwrite(&faces[i].group, sizeof(uint16_t), 1, output);
        fwrite(&group_masks[faces[i].group], sizeof(uint16_t), 1, output);
        fwrite(faces[i].vertex_index, sizeof(uint32_t), 3, output);
    }

    // Border data
    for (size_t i = 0; i < border_count; i++)
    {
        fwrite(&border_groups[i], sizeof(uint16_t), 1, output);
        fwrite(&group_masks[border_groups[i]], sizeof(uint16_t), 1, output);

        fwrite(&border_lengths[i], sizeof(uint32_t), 1, output);
        fwrite(border_vertex_indices[i], sizeof(uint32_t), border_lengths[i], output);
    }

    fclose(output);
    printf("Wrote %d triangles and %d borders to %s\n", face_count, border_count, output_filename);
}

#pragma mark Main
int main(int argc, const char * argv[])
{
    if (argc < 3)
        return 1;

    walkmap_convert_obj(argv[1], argv[2]);

//    if (argc < 3)
//    {
//        printf("Usage: GPModelConverter frame1.obj [frame2.obj [...]] output.mdl");
//        return 0;
//    }
//
//    model_convert_obj(&argv[1], argc - 2, argv[argc - 1]);
    return 0;
}
