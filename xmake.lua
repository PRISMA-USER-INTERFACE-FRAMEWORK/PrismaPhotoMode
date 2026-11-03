set_xmakever("3.0.9")

local commonlib_path = os.getenv("COMMONLIBF4_PATH")
if not commonlib_path then
    for _, candidate in ipairs({
        "../../../../../CommonLibF4",
        "../CommonLibF4",
        "../../CommonLibF4"
    }) do
        if os.isdir(path.join(os.scriptdir(), candidate)) then
            commonlib_path = candidate
            break
        end
    end
end

if not commonlib_path then
    raise("CommonLibF4 not found. Set COMMONLIBF4_PATH to its directory.")
end

includes(commonlib_path)

set_project("PrismaPhotoMode")
set_version("1.1.0")
set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")
set_toolchains("clang-cl[llvm]")

add_rules("mode.debug", "mode.release")
add_requires("spdlog v1.16.0")
add_requires("directxtex 2025.07", {configs = {dx12 = false}})

if is_host("linux") then
    add_requireconfs("**.cmake", {system = true, override = true})
end

target("PrismaPhotoMode")
    set_kind("shared")
    set_arch("x64")
    set_filename("PrismaPhotoMode.dll")
    set_runtimes("MD")

    add_deps("commonlibf4")
    add_packages("spdlog", "directxtex")

    add_files("src/PhotoMode.cpp")
    add_files("src/Screenshot.cpp")
    add_files("src/ScreenshotImage.cpp")
    add_files("src/PhotoModeLists.cpp")
    add_files("src/RuntimeSupport.cpp")
    add_files("src/main.cpp")
    add_headerfiles("src/*.h")
    add_includedirs("src")
    set_pcxxheader("src/PCH.h")

    add_defines("COMMONLIB_RUNTIMECOUNT=3", "F4SE_EXPORTS", "WIN32_LEAN_AND_MEAN", "NOMINMAX")
    add_syslinks("bcrypt", "version", "comctl32", "d3d11", "windowscodecs", "ole32", "shell32", "uuid")
    set_targetdir("$(builddir)/windows/x64/$(mode)")

    if is_mode("release") then
        set_optimize("fastest")
    end


target("ScreenshotBlendTest")
    set_kind("binary")
    set_runtimes("MD")
    set_default(false)
    add_files("tests/screenshot_blend.cpp")
    add_includedirs("src")
    set_targetdir("$(builddir)/windows/x64/$(mode)")


target("ScreenshotImageTest")
    set_kind("binary")
    set_runtimes("MD")
    set_default(false)
    add_packages("directxtex")
    add_files("tests/screenshot_image.cpp", "src/ScreenshotImage.cpp")
    add_includedirs("src")
    add_syslinks("d3d11", "windowscodecs", "ole32")
    set_targetdir("$(builddir)/windows/x64/$(mode)")
