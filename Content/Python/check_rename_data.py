import unreal
doc = unreal.AssetRenameData.__doc__
unreal.log("DOC_START")
for line in (doc or "").splitlines():
    unreal.log(f"  {line}")
unreal.log("DOC_END")
doc2 = unreal.AssetTools.rename_assets.__doc__
unreal.log("DOC2_START")
for line in (doc2 or "").splitlines():
    unreal.log(f"  {line}")
unreal.log("DOC2_END")
