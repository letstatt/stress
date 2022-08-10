import os, re

from utils.structs import Section, Test, TestProgress

def load_sections() -> list[Section]:
    sections = []
    ids = set()

    for entry in os.scandir('./tests'):
        if entry.is_dir():
            m = re.match(r".*?(\d+)_(.*)", entry.path)
            if m and len(m.groups()) == 2:
                g = m.groups()
                sections.append(Section(g[1], entry))
                ids.add(g[1])

    assert len(ids) == len(sections), "non-unique sections' ids assertion"
    return sections


def load_tests() -> list[Section]:
    sections = load_sections()

    for s in sections:
        ids = set()

        for entry in os.scandir(s.entry):
            if entry.is_dir():
                m = re.match(r".*?(\d+)_(.*)", entry.path[len(s.entry.path):])
                if m and len(m.groups()) == 2:
                    test_loader = None
                    for entry2 in os.scandir(entry):
                        if entry2.is_file() and re.match(r".*?[/\\]test.py", entry2.path):
                            assert test_loader is None, "regexp recognized two test loaders in " + entry.path
                            test_loader = entry2
                            break

                    if test_loader is not None:
                        g = m.groups()
                        s.addTest(Test(g[1], test_loader))
                        ids.add(g[1])

        assert len(ids) == len(s.tests),\
            "non-unique tests' ids assertion in section " + s.name
    return sections
