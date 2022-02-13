#include "base/rm200x_fs.h"

/******************************************************************************************/
// RM200x FILE SYSTEM
/******************************************************************************************/

//Task Tags
const char *FS_TASK_TAG = "FS_TASK";

//local globals
static esp_vfs_spiffs_conf_t conf;

//Local prototypes
void fs_example1(esp_vfs_spiffs_conf_t *pt_conf);
void fs_example2(esp_vfs_spiffs_conf_t *pt_conf);
void fs_example3(esp_vfs_spiffs_conf_t *pt_conf);

/******************************************************************************************/
// RM200x FILE SYSTEM - Definitions
/******************************************************************************************/

/******************************************************************************************/
// RM200x FILE SYSTEM
/******************************************************************************************/
void fs_test (void)
{
    // Call the examples and it will end the process and close
    //fs_example1(&conf);
    //fs_example2(&conf);
    fs_example3(&conf);
}

void fs_initialise(void)
{
    ESP_LOGI(FS_TASK_TAG, "Initializing SPIFFS");

    conf = 
    (esp_vfs_spiffs_conf_t)
    {
        .base_path = BASE_PATH,
        .partition_label = NULL,
        .max_files = MAX_FILES,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(FS_TASK_TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(FS_TASK_TAG, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(FS_TASK_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(FS_TASK_TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void fs_finalise(void)
{
    if (esp_spiffs_mounted((char *)&conf.partition_label))
    {
        // All done, unmount partition and disable SPIFFS
        esp_vfs_spiffs_unregister((char *)&conf.partition_label);
        ESP_LOGI(FS_TASK_TAG, "SPIFFS unmounted");
    }
}

bool fs_rename(const char *pt_newname, const char *pt_oldname)
{
    // Check if destination file exists before renaming
    struct stat st;
    if (stat(pt_newname, &st) == 0) 
    {
        // Delete it if it exists
        unlink(pt_newname);
    }

    // NOW - Rename original file
    ESP_LOGI(FS_TASK_TAG, "Renaming file %s to %s",pt_oldname, pt_newname);
    if (rename(pt_oldname, pt_newname) != 0)
    {
        ESP_LOGE(FS_TASK_TAG, "Rename failed");
        return false;
    }

    return true;
}

bool fs_delete(const char *pt_fliename)
{
    // Delete or unlink the file
    // Check if destination file exists before deleting
    struct stat st;
    if (stat(pt_fliename, &st) == 0) 
    {
        // Delete it if it exists
        unlink(pt_fliename);
        ESP_LOGI(FS_TASK_TAG, "Deleted file %s",pt_fliename);
        return true;
    }
    ESP_LOGI(FS_TASK_TAG, "File %s not found",pt_fliename);
    return false;
}

bool fs_write(const char *pt_fliename, const char *pt_text)
{
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(FS_TASK_TAG, "Opening file - WRITE");
    FILE *f = fopen(pt_fliename, "w");      //"w" for write operation
    if (f == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to open file for writing");
        return false;
    }
    fprintf(f, pt_text); 
    fclose(f);
    ESP_LOGI(FS_TASK_TAG, "File written");

    return true;
}

bool fs_append(const char *pt_fliename, const char *pt_text)
{
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(FS_TASK_TAG, "Opening file - APPEND");
    FILE *f = fopen(pt_fliename, "a");      //"a" for append operation
    if (f == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to open file for appending");
        return false;
    }
    fprintf(f, pt_text);
    fclose(f);
    ESP_LOGI(FS_TASK_TAG, "File written");

    return true;
}

bool fs_read(const char *pt_fliename, char *pt_text, size_t text_length)
{
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(FS_TASK_TAG, "Opening file - READ");
    FILE *f = fopen(pt_fliename, "r");      //"r" for read operations
    if (f == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to open file for reading");
        return false;
    }
    //Clear the read buffer
    memset(pt_text, 0x00, text_length);
    // Get the file length
    unsigned long length = fs_size(pt_fliename);
    // Read in the file data 
    // (small so we can do it in one go)
    fread(pt_text, sizeof(char), length, f);

    fclose(f);

    return true;
}

bool fs_read_next_line(const char *pt_fliename, char *pt_text, size_t text_length)
{
    static fpos_t fpos = 0;
    bool _is_open = false;
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(FS_TASK_TAG, "Opening file - READ");
    FILE *f = fopen(pt_fliename, "r");      //"r" for read operations
    if (f == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to open file for reading");
        _is_open = false;
        goto failed;
    }
    else
    {
        _is_open = true;
    }

    //move the file pointer to the correct item start (NB: bytes)
    if (fseek(f, fpos, SEEK_SET))
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to find item in file for reading");
        goto failed;
    }
    //Has EOF been reached
    if (feof(f))
    {
        ESP_LOGI(FS_TASK_TAG, "EOF reached - return");
        goto failed;
    }

    //Clear the read buffer
    memset(pt_text, 0x00, text_length);
    //Read the next line
    char* red = fgets((char *)pt_text, text_length, f);

    //get file position
    if (fgetpos(f, &fpos))
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to get position in open file");
        goto failed;
    }

    //Will EOF be reached next?
    if (feof(f))
    {
        //return quietly
        goto failed;
    }

    //Check if error
    if(red == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Line read failed");
        goto failed;
    }
    else
    {
        goto passed;
    }

/**********************************************************************************/
/*  RETURN AFTER CLOSING THE OPEN FILE                                            */
/**********************************************************************************/
passed:     // PASSED LABEL - close file if open and return true
    if (_is_open)
    {
        fclose(f);
        ESP_LOGI(FS_TASK_TAG, "%s - CLOSED", pt_fliename);
    }
    return true;

/**********************************************************************************/
failed:     // FAILED LABEL - clse file if open and return false
    if (_is_open)
    {
        fclose(f);
        ESP_LOGI(FS_TASK_TAG, "%s - CLOSED", pt_fliename);
    }
    //reset item counter to 0
    fpos = 0;
    return false;
/**********************************************************************************/
}

bool fs_write_struct(const char *pt_fliename, const void *pt_data, size_t strut_size)
{
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(FS_TASK_TAG, "Opening file - WRITE (struct)");
    FILE *f = fopen(pt_fliename, "wb");      //"wb" for binary write operation
    if (f == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to open file for writing struct");
        return false;
    }
    size_t leng_w = fwrite(pt_data, strut_size, 1, f);
    fclose(f);
    ESP_LOGI(FS_TASK_TAG, "%d items written to file %s - struct size = %d", leng_w, pt_fliename, strut_size);

    return true;
}

bool fs_append_struct(const char *pt_fliename, const void *pt_data, size_t strut_size)
{
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(FS_TASK_TAG, "Opening file - APPEND (struct)");
    FILE *f = fopen(pt_fliename, "ab");      //"ab" for binary append operation
    if (f == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to open file for appending struct");
        return false;
    }
    size_t leng_w = fwrite(pt_data, strut_size, 1, f);
    fclose(f);
    ESP_LOGI(FS_TASK_TAG, "%d item appended to file %s - struct size = %d", leng_w, pt_fliename, strut_size);

    return true;
}

bool fs_read_struct(const char *pt_fliename, void *pt_data, size_t strut_size, int r_item)
{
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(FS_TASK_TAG, "Opening file - READ (struct)");
    FILE *f = fopen(pt_fliename, "rb");      //"r" for read binary operations
    if (f == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to open file for reading");
        return false;
    }
    //Clear the read buffer
    memset(pt_data, 0x00, strut_size);
    //move the file pointer to the correct item start (NB: bytes)
    int offset = strut_size * r_item;
    if (fseek(f, offset, SEEK_SET))
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to find item in file for reading");
        return false;
    }
    // Read in the file data 
    int red = fread(pt_data, strut_size, 1, f);
    fclose(f);
    //Check id read is ok - should be at least 1 byte
    if (red > 0)
    {
        ESP_LOGI(FS_TASK_TAG, "Read %d item from %s", red, pt_fliename);
        return true;
    }
    return false;
}

bool fs_read_next_struct(const char *pt_fliename, void *pt_data, size_t strut_size)
{
    static int _item = 0;
    bool _is_open = false;
    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(FS_TASK_TAG, "Opening file - READ (struct)");
    FILE *f = fopen(pt_fliename, "rb");      //"rb" for read binary operations
    if (f == NULL)
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to open file for reading");
        _is_open = false;
        goto failed;
    }
    else
    {
        _is_open = true;
    }

    //Clear the read buffer
    memset(pt_data, 0x00, strut_size);
    //move the file pointer to the correct item start (NB: bytes)
    int offset = strut_size * _item;
    if (fseek(f, offset, SEEK_SET))
    {
        ESP_LOGE(FS_TASK_TAG, "Failed to find item in file for reading");
        goto failed;
    }
    // Read in the file data 
    int red = fread(pt_data, strut_size, 1, f);

    //Check id read is ok - should be at least 1 byte / 1 item
    if (red > 0)
    {
        ESP_LOGI(FS_TASK_TAG, "Read %d item # from %s", _item, pt_fliename);
        //if a sucessful read increment the _item counter
        _item++;
        goto passed;
    }
    else
    {
        goto failed;
    }

/**********************************************************************************/
/*  RETURN AFTER CLOSING THE OPEN FILE                                            */
/**********************************************************************************/
passed:     // PASSED LABEL - close file if open and return true
    if (_is_open)
    {
        fclose(f);
        ESP_LOGI(FS_TASK_TAG, "%s - CLOSED", pt_fliename);
    }
    return true;

/**********************************************************************************/
failed:     // FAILED LABEL - clse file if open and return false
    if (_is_open)
    {
        fclose(f);
        ESP_LOGI(FS_TASK_TAG, "%s - CLOSED", pt_fliename);
    }
    //reset item counter to 0
    _item = 0;
    return false;
/**********************************************************************************/
}


unsigned long fs_size(const char *pt_filename)
{
    FILE * f = fopen(pt_filename, "r");
    fseek(f, 0, SEEK_END);                          //Move position to end of the file
    unsigned long len = (unsigned long)ftell(f);    //Get the current position == EOF
    fseek(f, 0, SEEK_SET);                          //Move position to beginning of the file
    fclose(f);
    return len;
}

void fs_example1(esp_vfs_spiffs_conf_t *pt_conf)
{
    fs_write(CONFIG_FILENAME, "Hello World - how are you on Monday?\n");
    fs_append(CONFIG_FILENAME, "Hello World - how are you on Tuesday?\n");
    fs_append(CONFIG_FILENAME, "Hello World - how are you on Wednesday?\n");
    fs_append(CONFIG_FILENAME, "Hello World - how are you on Thursday?\n");
    fs_append(CONFIG_FILENAME, "Hello World - how are you on Friday?\n");
    fs_append(CONFIG_FILENAME, "Hello World - how are you on Satuday?\n");
    fs_append(CONFIG_FILENAME, "Hello World - how are you on Sunday?\n");

    int _line_counter = 0;
    char _str[1024];
    while(fs_read_next_line(CONFIG_FILENAME, (char *)&_str, sizeof(_str)))
    {
        ESP_LOGI(FS_TASK_TAG, "%d) %s", _line_counter++, _str);
    }
}

void fs_example2(esp_vfs_spiffs_conf_t *pt_conf)
{
    uart_message_t data;

    data =
    (uart_message_t)
    {
        .data = { 0xff, 0x55, 0x01, 0x03, 0x94, 0xd6, 0x12, 0x85 },
        .length = 8,
        .IsASCII = false,
        .IsHEX = true,
        .port = 2,
        .Message_ID = 0,
    };
    fs_write_struct(CONFIG_BIN_FILENAME, &data, sizeof(data));
    ESP_LOGI(FS_TASK_TAG, "File size = %lu", fs_size(CONFIG_BIN_FILENAME));

    data =
    (uart_message_t)
    {
        .data = { 0xff, 0x55, 0x01, 0x03, 0x94, 0xd6, 0x12, 0x84 },
        .length = 8,
        .IsASCII = false,
        .IsHEX = true,
        .port = 2,
        .Message_ID = 1
    };
    fs_append_struct(CONFIG_BIN_FILENAME, &data, sizeof(uart_message_t));

    data =
    (uart_message_t)
    {
        .data = { 0xff, 0x55, 0x01, 0x03, 0x94, 0xd6, 0x12, 0x83 },
        .length = 8,
        .IsASCII = false,
        .IsHEX = true,
        .port = 2,
        .Message_ID = 2
    };
    fs_append_struct(CONFIG_BIN_FILENAME, &data, sizeof(uart_message_t));

    data =
    (uart_message_t)
    {
        .data = { 0xff, 0x55, 0x01, 0x03, 0x94, 0xd6, 0x12, 0x82 },
        .length = 8,
        .IsASCII = false,
        .IsHEX = true,
        .port = 2,
        .Message_ID = 3
    };
    fs_append_struct(CONFIG_BIN_FILENAME, &data, sizeof(uart_message_t));

    while (fs_read_next_struct(CONFIG_BIN_FILENAME, &data, sizeof(uart_message_t)))
    {
        //Output some of the reply data to confirm
        ESP_LOGI(FS_TASK_TAG, "0x%x item from port %d - msg length = %d", data.data[7], data.port, data.length);
    }


    //fs_finalise();
}

void fs_example3(esp_vfs_spiffs_conf_t *pt_conf)
{
    int _line_counter = 0;
    char _str[1024];
    while(fs_read_next_line(CONFIG_FILENAME, (char *)&_str, sizeof(_str)))
    {
        printf("(%d) %s", _line_counter++, _str);
        //ESP_LOGI(FS_TASK_TAG, "%d) %s", _line_counter++, _str);
    }

    uart_message_t data;
    _line_counter = 0;
    while (fs_read_next_struct(CONFIG_BIN_FILENAME, &data, sizeof(uart_message_t)))
    {
        //Output some of the reply data to confirm
        printf("(%d) : 0x%x from port %d - msg length = %d\r\n", _line_counter++, data.data[7], data.port, data.length);
        //ESP_LOGI(FS_TASK_TAG, "0x%x item from port %d - msg length = %d", data.data[7], data.port, data.length);
    }
}


/******************************************************************************************/
// RM200x FILE SYSTEM   -- END --
/******************************************************************************************/

/******************************************************************************************/
// REFERENCE - fopen
/******************************************************************************************/
/*
    "r"     Opens a file for reading. The file must exist.
    "w"     Creates an empty file for writing. If a file with the same name already exists, 
            its content is erased and the file is considered as a new empty file.
    "a"     Appends to a file. Writing operations, append data at the end of the file. The file is created if it does not exist.
    "r+"    Opens a file to update both reading and writing. The file must exist.
    "w+"    Creates an empty file for both reading and writing.
    "a+"    Opens a file for reading and appending

    "xxb"   Add a suffix b for binary (data) file operations

    Seek constants
    # define	SEEK_SET	0   //Beginning of file
    # define	SEEK_CUR	1   //Current position
    # define	SEEK_END	2   //End of file

*/
/******************************************************************************************/