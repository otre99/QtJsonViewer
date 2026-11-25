/*
 * Install script for the QtJsonViewer component.
 *
 * Installed layout (relative to @TargetDir@):
 *   bin/QtJsonViewer(.exe)
 *   lib/, plugins/            (Qt runtime + bundled libs, Linux)
 *   share/icons/QtJsonViewer.png
 */

function Component()
{
}

Component.prototype.createOperations = function()
{
    // Extract the component data (bin/, lib/, plugins/, share/ ...).
    component.createOperations();

    if (systemInfo.kernelType === "linux") {
        var exe = "@TargetDir@/bin/QtJsonViewer";
        var icon = "@TargetDir@/share/icons/QtJsonViewer.png";

        // A relative file name puts the entry in ~/.local/share/applications
        // (or /usr/share/applications when installing as root).
        // The entry is removed automatically on uninstall.
        component.addOperation("CreateDesktopEntry",
            "QtJsonViewer.desktop",
            "Type=Application\n" +
            "Name=QtJsonViewer\n" +
            "GenericName=JSON Viewer\n" +
            "Comment=Fast viewer for large JSON documents\n" +
            "Exec=\"" + exe + "\" %f\n" +
            "TryExec=" + exe + "\n" +
            "Icon=" + icon + "\n" +
            "Terminal=false\n" +
            "MimeType=application/json;\n" +
            "Categories=Development;Utility;\n" +
            "StartupWMClass=QtJsonViewer\n");
    } else if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut",
            "@TargetDir@/bin/QtJsonViewer.exe",
            "@StartMenuDir@/QtJsonViewer.lnk",
            "workingDirectory=@TargetDir@/bin",
            "description=Fast viewer for large JSON documents");
        component.addOperation("CreateShortcut",
            "@TargetDir@/QtJsonViewerMaintenanceTool.exe",
            "@StartMenuDir@/Uninstall QtJsonViewer.lnk",
            "workingDirectory=@TargetDir@",
            "description=Uninstall QtJsonViewer");
    }
}
