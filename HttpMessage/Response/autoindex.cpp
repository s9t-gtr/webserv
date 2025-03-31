#include "Response.hpp"
#include <dirent.h>

static std::string encodeToHyperlink(const struct dirent *entry)
{
    std::string index_name = std::string(entry->d_name);
    if (entry->d_type == DT_DIR)
        index_name.append("/");
    std::stringstream ss;
    std::string escape_chars = " \"<>\\^`{|}#%&+;?";

    for (size_t i = 0; i < index_name.size(); i++)
    {
        char c = index_name.at(i);
        if ((0x0 <= c && c <= 0x1F) || (0x7F <= c) || escape_chars.find(c) != std::string::npos)
            ss << "%" << std::hex << (int)c;
        else
            ss << c;
    }
    return (ss.str());
}

// エントリの名前が50文字を超えたら以降省略して出力させる関数
static std::string getDisplayedName(const struct dirent *entry)
{
    std::string index_name = std::string(entry->d_name);
    if (entry->d_type == DT_DIR)
        index_name.append("/");
    if (index_name.length() > 50)
        index_name = index_name.substr(0, 50 - 3).append("..>");
    return (index_name);
}

// "last_modified"項目に出力される内容を取得する関数
static std::string getDisplayedDate(const struct dirent *entry, std::string path_fixed)
{
    std::string path(path_fixed);
    path += entry->d_name;
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) < 0)
        perror("stat");
    char date_str[256];
    struct tm *mtime = gmtime(&path_stat.st_mtimespec.tv_sec);
    strftime(date_str, 256, "%d-%b-%Y %H:%M", mtime);

    return (std::string(date_str));
}

// "size"項目に出力される内容を取得する関数
static std::string getDisplayedSize(const struct dirent *entry, std::string path_fixed)
{
    std::string path(path_fixed);
    path += entry->d_name;
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) < 0)
        perror("stat");
    std::stringstream ss;
    if (S_ISDIR(path_stat.st_mode))
        ss << "-";
    else
        ss << path_stat.st_size;

    return (ss.str());
}
#define UPLOAD "upload/"
// index_listの内容ごとに変える必要のある行を作成する関数
static std::string getIndexList(std::string path)
{
    DIR *dir = opendir(path.c_str());
    if (dir == NULL)
        perror("opendir");
    struct dirent *entry;
    std::string index_list = "";
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        index_list += "<tr>\n";
        index_list += "<td align=\"left\" width=\"150\"><a href=\"" + encodeToHyperlink(entry) + "\">" + getDisplayedName(entry) + "</a></td>\n";
        index_list += "<td align=\"center\" width=\"200\">" + getDisplayedDate(entry, path) + "</td>\n";
        index_list += "<td align=\"center\" width=\"100\">" + getDisplayedSize(entry, path) + "</td>\n";
        if(path == UPLOAD){
            index_list += "<td align=\"center\" width=\"100\">";
            index_list += "<button onclick=\"deleteFile('" + getDisplayedName(entry) + "')\">delete</button>";
        }
        index_list += "</tr>\n";
    }
    closedir(dir);
    return (index_list);
}

std::string Response::createAutoindexPage(Request& requestInfo){
    std::string content;
    content = "<html>\n";
    content += "<head>\n";
    content += "<title>Index of " + requestInfo.getRequestTarget() + "</title>\n";
    if(requestInfo.getPath() == UPLOAD){
        content += "<script>function deleteFile(fileName) {if(confirm(fileName + ' delete OK?')) {fetch(fileName, {method: 'DELETE',}).then(response => {if (response.ok) {alert('deleted'); location.reload();} else {alert('failed delete file'); } }) .catch(error => { console.error('Error:', error); alert('error');});}}</script>";
    }
    content += "</head>\n";
    content += "<body>\n";
    content += "<h1>Index of " + requestInfo.getRequestTarget()  + "</h1>\n";
    content += "<hr><table>\n";
    content += "<tr>\n";
    content += "<td align=\"left\" width=\"150\">index_list</td>\n";
    content += "<td align=\"center\" width=\"200\">last_modified</td>\n";
    content += "<td align=\"center\" width=\"100\">size</td>\n";
    content += "</tr>\n";
    content += "<tr>\n";
    content += "<td align=\"left\" width=\"150\"><a href=\"../\">../</a></td>\n";
    content += "</tr>\n";
    content += getIndexList(requestInfo.getPath());
    content += "</table><hr>\n";
    content += "</body>\n";
    content += "</html>\n";

    return content;
}
