#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include <string>
#include <vector>
#include "portable-file-dialogs.h" // Ensure this header is in your include path

namespace FileDialog {

    // Opens a file dialog and returns the selected file paths.
    // 'title' specifies the window title.
    // 'defaultPath' specifies the default path (can be empty).
    // 'filters' is a vector of pairs; each pair contains a description and a file pattern.
    // Example usage:
    //   auto files = FileDialog::openFile("Select a NIfTI File", "", {{"NIfTI Files", "*.nii"}});
    inline std::vector<std::string> openFile(const std::string& title = "Open File",
                                               const std::string& defaultPath = "",
                                               const std::vector<std::pair<std::string, std::string>>& filters = {}) {
        std::vector<std::string> filterList;
        // If filters provided, flatten the pairs into a vector of strings:
        if (!filters.empty()) {
            for (const auto& f : filters) {
                filterList.push_back(f.first);
                filterList.push_back(f.second);
            }
        }
        // Otherwise, use a default filter to show all files.
        else {
            filterList = {"All Files", "*"};
        }
        
        // pfd::open_file opens the dialog and returns a vector of selected file paths.
        return pfd::open_file(title, defaultPath, filterList).result();
    }

} // namespace FileDialog

#endif // FILE_DIALOG_H
