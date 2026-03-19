#include "game/obj_file.h"

#include "game/obj.h"
#include "game/obj_private.h"

static bool append_handle_fname(int64_t handle, char* path);
static TigFile* open_solitary_for_write(int64_t handle, const char* dir, const char* ext);
static bool handle_from_fname(int64_t* handle_ptr, const char* path);

/**
 * Writes an object to a solitary object file int the specified directory with
 * the given extension.
 *
 * 0x4E6B60
 */
bool objf_solitary_write(int64_t handle, const char* dir, const char* ext)
{
    TigFile* stream;

    // Open the file for writing.
    stream = open_solitary_for_write(handle, dir, ext);
    if (stream == NULL) {
        return false;
    }

    // Write one object to the file.
    if (!obj_write(stream, handle)) {
        tig_file_fclose(stream);
        return false;
    }

    // Close the file.
    tig_file_fclose(stream);

    return true;
}

/**
 * Reads an object from a solitary object file.
 *
 * 0x4E6BC0
 */
bool objf_solitary_read(int64_t* handle_ptr, const char* path)
{
    TigFile* stream;

    // Open the file for reading.
    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        tig_debug_printf("Unable to open [%s] in ObjFile, objf_solitary_read\n", path);
        return false;
    }

    // Read one object from the file.
    if (!obj_read(stream, handle_ptr)) {
        tig_file_fclose(stream);
        return false;
    }

    // Close the file.
    tig_file_fclose(stream);

    return true;
}

/**
 * Deletes an object's solitary file and creates a corresponding `.del` file.
 *
 * 0x4E6D10
 */
void objf_solitary_delete(int64_t handle, const char* dir, const char* ext)
{
    char path[TIG_MAX_PATH];
    int length;
    char* dot;
    TigFile* stream;

    strcpy(path, dir);

    if (!append_handle_fname(handle, path)) {
        tig_debug_println("Can't generate filename in ObjFile, objf_solitary_delete");
        return;
    }

    strcat(path, ext);

    // NOTE: Such validation looks meaningless, stack is most likely already
    // ruined.
    length = (int)strlen(path);
    if (length > TIG_MAX_PATH) {
        tig_debug_println("Filename too long in ObjFile, objf_solitary_delete.");
        return;
    }

    // Check if the file exists and attempt to delete it.
    if (tig_file_exists(path, NULL) && tig_file_remove(path) != 0) {
        tig_debug_printf("Unable to delete [%s] in ObjFile, objf_solitary_delete\n", path);
        return;
    }

    // Replace the extension with `.del` to create a deletion marker file.
    dot = strchr(path, '.');
    if (dot == NULL) {
        tig_debug_printf("Unable to find the dot in [%s] in ObjFile, objf_solitary_delete\n", path);
        return;
    }

    strcpy(dot, ".del");

    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        tig_debug_printf("Unable to open [%s] in ObjFile, objf_solitary_delete\n", path);
        return;
    }

    // Write the path length to the `.del` file.
    if (!objf_write(&length, sizeof(length), stream)) {
        tig_debug_println("Write failed in ObjFile, objf_solitary_delete");
    }

    tig_file_fclose(stream);
}

/**
 * Writes a buffer to a file stream.
 *
 * 0x4E7010
 */
bool objf_write(const void* buffer, size_t size, TigFile* stream)
{
    return tig_file_fwrite(buffer, size, 1, stream) == 1;
}

/**
 * Reads a buffer from a file stream.
 *
 * 0x4E7030
 */
bool objf_read(void* buffer, size_t size, TigFile* stream)
{
    return tig_file_fread(buffer, size, 1, stream) == 1;
}

/**
 * Appends an object handle's ID string to a file path.
 *
 * 0x4E7050
 */
bool append_handle_fname(int64_t handle, char* path)
{
    size_t length;

    // Ensure the path ends with a path separator.
    length = strlen(path);
    if (length != 0 && path[length - 1] != '\\') {
        path[length] = '\\';
        path[length + 1] = '\0';
        length++;
    }

    // Append the object ID string to the path.
    objid_id_to_str(path + length, obj_get_id(handle));

    return true;
}

/**
 * Opens a file for writing a solitary object's data.
 *
 * 0x4E70C0
 */
TigFile* open_solitary_for_write(int64_t handle, const char* dir, const char* ext)
{
    char path[TIG_MAX_PATH];
    int length;
    TigFile* stream;

    strcpy(path, dir);

    if (!append_handle_fname(handle, path)) {
        return NULL;
    }

    strcat(path, ext);

    // NOTE: Such validation looks meaningless, stack is most likely already
    // ruined.
    length = (int)strlen(path);
    if (length > TIG_MAX_PATH) {
        tig_debug_println("Filename too long in ObjFile, open_solitary_for_write.");
        return NULL;
    }

    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        tig_debug_printf("Unable to open [%s] in ObjFile, open_solitary_for_write\n", path);
        return NULL;
    }

    return stream;
}

/**
 * Extracts an object handle from a file name in the given path.
 *
 * 0x4E71B0
 */
bool handle_from_fname(int64_t* handle_ptr, const char* path)
{
    char fname[TIG_MAX_PATH];
    ObjectID oid;

    // Retrieve file name.
    compat_splitpath(path, NULL, NULL, fname, NULL);

    if (fname[0] == '\0') {
        tig_debug_printf("handle_from_fname found empty base_name infilename [%s]", path);
        return false;
    }

    // Convert the file name to an ObjectID.
    if (!objid_id_from_str(&oid, fname)) {
        tig_debug_printf("Unable to extract id from filename [%s]\n", path);
        return false;
    }

    // Validate the ID type.
    if (oid.type <= 0 || oid.type > 3) {
        tig_debug_printf("Wrong id type in filename [%s]\n", path);
        return false;
    }

    // Lookup the object handle for ID.
    *handle_ptr = obj_pool_perm_lookup(oid);
    if (*handle_ptr == OBJ_HANDLE_NULL) {
        tig_debug_printf("ID not loaded, can't convert to handle in ObjFile,handle_from_fname: [%s]", fname);
        return false;
    }

    return true;
}
