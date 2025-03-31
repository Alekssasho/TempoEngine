use nom::branch::alt;
use nom::bytes::complete::tag;
use nom::bytes::complete::take;
use nom::bytes::complete::take_until;
use nom::character::complete::alpha1;
use nom::character::complete::alphanumeric1;
use nom::character::complete::line_ending;
use nom::character::complete::multispace0;
use nom::character::complete::not_line_ending;
use nom::combinator::map;
use nom::combinator::recognize;
use nom::error::ParseError;
use nom::multi::many0;
use nom::multi::many0_count;
use nom::multi::separated_list0;
use nom::sequence::delimited;
use nom::sequence::pair;
use nom::sequence::tuple;
use nom::IResult;
use nom::Parser;

use crate::structures::*;

extern crate nom;

fn ws<'a, O, E: ParseError<&'a str>, F>(inner: F) -> impl FnMut(&'a str) -> IResult<&'a str, O, E>
where
    F: Parser<&'a str, O, E>,
{
    delimited(multispace0, inner, multispace0)
}

fn identifier(input: &str) -> IResult<&str, &str> {
    recognize(pair(
        alt((alpha1, tag("_"))),
        many0_count(alt((alphanumeric1, tag("_"), tag("::"), tag("*"), tag("<"), tag(">"), tag(",")))),
    ))
    .parse(input)
}

fn parse_single_member_attribute(input: &str) -> IResult<&str, MemberAttribute> {
    alt((map(
        tuple((tag("StorageType("), identifier, tag(")"))),
        |(_, ttype, _)| MemberAttribute::StorageType(ttype.to_string()),
    ),))(input)
}

fn parse_member_attributes(input: &str) -> IResult<&str, Vec<MemberAttribute>> {
    let (input, _) = tag("[")(input)?;
    let (_, input) = take_until("]")(input)?;
    separated_list0(tag(","), parse_single_member_attribute)(input)
}

fn parse_member(input: &str) -> IResult<&str, Member> {
    let input = check_and_skip_comment(input);
    let (input, attributes) = if let Ok((left_input, attributes_input)) = recognize(tuple((
        tag::<&str, &str, nom::error::Error<&str>>("["),
        take_until("]"),
        take(1u32),
    )))(input)
    {
        let (_, attributes) = parse_member_attributes(attributes_input)?;
        (left_input, attributes)
    } else {
        (input, vec![])
    };

    let (input, (name, _, ttype, _)) =
        ws(tuple((ws(identifier), tag(":"), ws(identifier), tag(";"))))(input)?;
    Ok((
        input,
        Member {
            name: name.to_string(),
            ttype: ttype.to_string(),
            attributes,
        },
    ))
}

fn parse_structure_type(input: &str) -> IResult<&str, StructureType> {
    ws(alt((
        map(tag("Component"), |_| StructureType::Component),
        map(tag("Tag"), |_| StructureType::Tag),
    )))(input)
}

fn parse_single_structure_attribute(input: &str) -> IResult<&str, StructureAttribute> {
    alt((map(tag("NoReflection"), |_| {
        StructureAttribute::NoReflection
    }),))(input)
}

fn parse_structure_attributes(input: &str) -> IResult<&str, Vec<StructureAttribute>> {
    let (input, _) = tag("[")(input)?;
    let (_, input) = take_until("]")(input)?;
    separated_list0(tag(","), parse_single_structure_attribute)(input)
}

fn parse_structure(input: &str) -> IResult<&str, Structure> {
    let input = check_and_skip_comment(input);
    let (input, attributes) = if let Ok((left_input, attributes_input)) = recognize(tuple((
        tag::<&str, &str, nom::error::Error<&str>>("["),
        take_until("]"),
        take(1u32),
    )))(input)
    {
        let (_, attributes) = parse_structure_attributes(attributes_input)?;
        (left_input, attributes)
    } else {
        (input, vec![])
    };
    let (input, structure_type) = ws(parse_structure_type)(input)?;
    let (input, name) = identifier(input)?;

    let (input, (_, members, _)) = if let StructureType::Component = structure_type {
        tuple((ws(tag("{")), many0(parse_member), ws(tag("}"))))(input)?
    } else {
        let (input, _) = ws(tag(";"))(input)?;
        (input, ("", vec![], ""))
    };

    Ok((
        input,
        Structure {
            name: name.to_string(),
            members,
            attributes,
            ttype: structure_type,
            ..Default::default()
        },
    ))
}

fn parse_comment(input: &str) -> IResult<&str, &str> {
    let (input, (_, comment, _)) = tuple((ws(tag("//")), not_line_ending, line_ending))(input)?;
    Ok((input, comment))
}

fn check_and_skip_comment(input: &str) -> &str {
    let res = parse_comment(input);
    if let Ok((left_input, _)) = res {
        left_input
    } else {
        input
    }
}

pub fn parse_input(input: &str) -> IResult<&str, Vec<Structure>> {
    many0(parse_structure)(input)
}
