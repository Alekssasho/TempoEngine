use serde::Serialize;
use serde::Deserialize;

#[derive(Debug, Serialize, Deserialize)]
pub struct Member {
    pub name: String,
    pub ttype: String,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Structure {
    pub name: String,
    pub members: Vec<Member>,
    pub attributes: Vec<Attribute>,
}

#[derive(Debug, Serialize, Deserialize)]
pub enum Attribute {
    Component,
}
