import sys
from pathlib import Path
# sys.path.append("/home/wupuqing/workspace/PIM-ANNS/build")
sys.path.append(str(Path(__file__).resolve().parent / "build"))
import cmake_example as m

m.build(0,0,0,0)

m.load("xxx")

# m.search()

