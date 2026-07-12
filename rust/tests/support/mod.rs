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

#[allow(dead_code)]
pub fn load_chunk_sizes(
    root: &Path,
    group: &str,
    name: &str,
    input_length: usize,
) -> io::Result<Vec<usize>> {
    let contents = fs::read_to_string(root.join(group).join(name).join("chunks.txt"))?;
    let sizes: Vec<usize> = contents
        .split_whitespace()
        .map(str::parse)
        .collect::<Result<_, _>>()
        .map_err(|error| io::Error::new(io::ErrorKind::InvalidData, error))?;

    if sizes.is_empty() || sizes.contains(&0) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "chunk sizes must be positive",
        ));
    }
    if sizes.iter().sum::<usize>() != input_length {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            "chunk sizes do not match input length",
        ));
    }

    Ok(sizes)
}
