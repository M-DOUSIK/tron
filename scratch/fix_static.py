import re
for f in ["C:/Users/Dousik/Workspace/TRON/sessions/session_08B/FSBL/Src/ai/detector.c", "C:/Users/Dousik/Workspace/TRON/sessions/session_08B/FSBL/Src/ai/embedder.c"]:
    with open(f, "r") as file:
        content = file.read()
    
    # Prepend static to void forward_lite_
    content = re.sub(r"^void forward_lite_", "static void forward_lite_", content, flags=re.MULTILINE)
    
    with open(f, "w") as file:
        file.write(content)
print("Done")

