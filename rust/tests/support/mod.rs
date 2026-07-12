use std::fs;
use std::io;
use std::path::Path;

pub struct Fixture {
    pub input: Vec<u8>,
    pub expected: Vec<u8>,
}

pub fn load(root: &Path, group: &str, name: &str) -> io::Result<Fixture> {
    let directory = root.join(group).join(name);
    Ok(Fixture {
        input: fs::read(directory.join("input.bin"))?,
        expected: fs::read(directory.join("expected.bin"))?,
    })
}
