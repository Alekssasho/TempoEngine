use serde::Deserialize;
use serde::Serialize;

#[derive(Default, Debug, Serialize, Deserialize)]
pub enum StructureType {
    #[default]
    Component,
    Tag,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct Member {
    pub name: String,
    pub ttype: String,
    pub attributes: Vec<MemberAttribute>,
}

#[derive(Default, Debug, Serialize, Deserialize)]
pub struct Structure {
    pub name: String,
    pub members: Vec<Member>,
    pub attributes: Vec<StructureAttribute>,
    pub ttype: StructureType,
}

#[derive(Debug, Serialize, Deserialize)]
pub enum StructureAttribute {
    NoReflection,
    Inherit,
}

#[derive(Debug, Serialize, Deserialize)]
pub enum MemberAttribute {
    StorageType(String),
}
