import os
from pathlib import Path

TEST_DIR = Path(__file__).resolve().parent.parent
THIS_DIR = Path(os.getcwd()).resolve()

from .runner import TestRunner # type: ignore # Re-export

