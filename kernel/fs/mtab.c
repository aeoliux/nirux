#include <device.h>
#include <driver.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fs.h>

struct fs fs_mounts[MAXFSMNTED + 1] = {};

ssize_t fs_get_mountpoint(const char *path, size_t *offset) {
    int not_mismatched = -1;
    size_t not_mismatched_length = 0, str_off = 0;
    for (size_t i = 0; i < MAXFSMNTED + 1; i++) {
        if (!fs_mounts[i].driver)
            continue;

        size_t mnt_len = strlen(fs_mounts[i].mountpoint);
        int mismatch = strnmis(path, fs_mounts[i].mountpoint, mnt_len);
        if (mismatch == -1 && not_mismatched_length <= mnt_len) {
            not_mismatched = i;
            not_mismatched_length = mnt_len;
            str_off = mnt_len;
        }
    }

    if (offset && not_mismatched != -1)
        *offset = str_off;

    return not_mismatched;
}

ssize_t fs_resolve_file(const char *pth, struct fs **mnt, struct fs_direntry *restrict file) {
    if (strlen(pth) > MAXPATHSIZ || *pth != '/')
        return -EINVAL; // path must be within length limit and must be absolute

    char p[MAXFNAMSIZ + 1];
    char *path = p;
    memcpy(path, pth, strlen(pth) + 1);

    size_t fs_relative_off = 0;
    int ind = fs_get_mountpoint(p, &fs_relative_off);
    if (ind < 0)
        return -ENOENT;

    file->type = FS_DIR;
    file->gid = 0;
    file->uid = 0;
    file->mode = 0644;
    file->parent_ref = 0;
    file->ref = 0;
    file->size = 0;
    file->last_access = 0;
    file->filename[0] = 0;

    *mnt = &fs_mounts[ind];
    path = p + fs_relative_off;
    if (*path == '/')
        path++;

    struct fs_direntry entry, parent;

    size_t start = 0, len = strlen(path);
    if (len) {
        for (size_t i = 0; i < len + 1; i++) {
            if (path[i] == '/' || path[i] == 0) {
                // do
                path[i] = 0;

                ssize_t result;
                size_t seek = 0;
                while ((result = (*mnt)->driver->fetch_entry((*mnt)->device, start ? &parent : NULL, &entry, &seek)) > 0) {
                    if (!strcmp(path + start, entry.filename))
                        break;
                }
            
                if (result < 0)
                    return result;
                
                if (!result)
                    return -ENOENT;

                memcpy(&parent, &entry, sizeof(entry));
                start = i + 1;
            }
        }

        memcpy(file, &entry, sizeof *file);
    }

    return 0;
}

int fs_is_dir(const char *path) {
    struct fs_direntry entry;
    struct fs *mnt;

    ssize_t result = fs_resolve_file(path, &mnt, &entry);
    return !result ? entry.type & FS_DIR : 0;
}

ssize_t fs_read_dir(const char *path, size_t *seek, struct fs_direntry *restrict out) {
    static char current_path[MAXPATHSIZ + 1];
    static struct fs_direntry dir;
    static struct fs *mnt;
    ssize_t result;

    if (strcmp(path, current_path)) {
        memset(current_path, 0, MAXPATHSIZ + 1);
        memset(&dir, 0, sizeof dir);
        mnt = NULL;

        result = fs_resolve_file(path, &mnt, &dir);
        if (result && result != -EISDIR)
            return result;

        strcpy(current_path, path);
    }

    if (dir.type & FS_DIR) {
        return mnt->driver->fetch_entry(mnt->device, dir.ref ? &dir : NULL, out, seek);
    } else {
        memcpy(out, &dir, sizeof *out);
    }

    return *seek;
}

ssize_t fs_read_path(const char *path, size_t seek, void *restrict buf, size_t n) {
    struct fs_direntry entry;
    struct fs *mnt;

    ssize_t result = fs_resolve_file(path, &mnt, &entry);
    if (result)
        return result;

    if (entry.type & FS_DIR)
        return -EISDIR;
    
    return mnt->driver->read(mnt->device, &entry, buf, seek, n);
}

int fs_mount(uint16_t device, const char *path, struct fs_driver *fsdrv) {
    if (strlen(path) > MAXPATHSIZ || *path != '/')
        return -EINVAL;

    struct fs *mnt = NULL;
    for (size_t i = 0; i < MAXFSMNTED; i++)
        if (!fs_mounts[i].driver)
            mnt = &fs_mounts[i];
    
    if (!mnt)
        return -ENOMEM;

    if (!fs_is_dir(path))
        return -ENOTDIR;

    int result = fsdrv->initialise(device);
    if (result >= 0) {
        mnt->device = device;
        mnt->driver = fsdrv;
        mnt->depends_on = fs_get_mountpoint(path, NULL);

        memcpy(mnt->mountpoint, path, strlen(path));

        return 0;
    }

    return result;
}

int fs_mount_root(uint16_t device, struct fs_driver *fsdrv) {
    if (fs_mounts[0].driver)
        return -EPERM;

    int result = fsdrv->initialise(device);
    if (result >= 0) {
        fs_mounts[0].device = device;
        fs_mounts[0].depends_on = -1;
        fs_mounts[0].driver = fsdrv;

        fs_mounts[0].mountpoint[0] = '/';
        fs_mounts[0].mountpoint[1] = 0;
    }

    return result;
}