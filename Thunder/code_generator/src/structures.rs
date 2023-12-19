#[derive(Debug)]
pub struct Member {
    pub name: String,
    pub ttype: String,
}

#[derive(Debug)]
pub struct Structure {
    pub name: String,
    pub members: Vec<Member>,
}
