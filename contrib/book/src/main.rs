use {
    std::{
        collections::{BTreeMap, BTreeSet},
        fs,
        path::Path,
    },
    walkdir::WalkDir,
};

const IGNORE: &[&str] = &[
    ".github",
    "contrib/book",
    "doc/release-notes",
    "src/crc32c",
    "src/leveldb",
    "src/minisketch",
    "src/secp256k1",
];

const README: &str = "README.md";

fn main() {
    let root = Path::new(env!("WORKSPACE_ROOT"));

    let mut chapters = BTreeMap::<String, BTreeSet<String>>::new();

    for entry in WalkDir::new(root) {
        let entry = entry.unwrap();

        let path = entry.path().strip_prefix(root).unwrap();

        if path.extension() != Some("md".as_ref())
            || IGNORE.iter().any(|prefix| path.starts_with(prefix))
        {
            continue;
        }

        let parent = path
            .parent()
            .map(|parent| parent.to_str().unwrap())
            .unwrap_or_default();

        let filename = path.file_name().unwrap().to_str().unwrap();

        chapters
            .entry(parent.into())
            .or_default()
            .insert(filename.into());
    }

    let mut summary = String::from("# Summary\n\n");

    let build = root.join("contrib/book/build");

    let src = build.join("src");

    fs::remove_dir_all(&src).ok();

    for (chapter, files) in chapters {
        let readme = if files.contains(README) {
            fs::read_to_string(root.join(&chapter).join(README)).unwrap()
        } else {
            "This Page Intentionally Left Blank".into()
        };

        let dst = src.join(&chapter);

        fs::create_dir_all(&dst).unwrap();

        fs::write(dst.join(README), &readme).unwrap();

        let level = chapter.chars().filter(|&c| c == '/').count();

        if chapter == "" {
            summary.push_str(&format!("[README]({README})\n"));
        } else {
            summary.push_str(&format!(
                "{}- [{}]({chapter}/{README})\n",
                "  ".repeat(level),
                chapter.split('/').last().unwrap(),
            ));
        };

        for file in files {
            if file == README {
                continue;
            }

            fs::copy(
                root.join(&chapter).join(&file),
                src.join(&chapter).join(&file),
            )
            .unwrap();

            let name = file.strip_suffix(".md").unwrap();

            if chapter == "" {
                summary.push_str(&format!("- [{name}]({file})\n"));
            } else {
                summary.push_str(&format!(
                    "{}- [{name}]({chapter}/{file})\n",
                    "  ".repeat(level + 1),
                ));
            }
        }
    }

    fs::write(src.join("SUMMARY.md"), summary).unwrap();
}
