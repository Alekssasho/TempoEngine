use std::{cell::Cell, cell::Ref, cell::RefCell, collections::HashMap, path::PathBuf};

use daggy::Walker;
use petgraph::graph::node_index;

use super::resources::{Resource, ResourceId};

#[derive(Clone)]
pub struct CompilerOptions {
    pub output_folder: PathBuf,
}

pub struct CompilerGraph {
    pub options: CompilerOptions,
    graph: daggy::Dag<usize, ()>,
    current_resource_id: Cell<usize>,
}

impl CompilerGraph {
    pub fn get_next_resource_id(&self) -> ResourceId {
        ResourceId(
            self.current_resource_id
                .replace(self.current_resource_id.get() + 1),
        )
    }
}

pub type ResourceBox = Box<dyn Resource>;

struct GraphNode {
    resource: ResourceBox,
    compiled_data: RefCell<Vec<u8>>,
}

impl GraphNode {
    fn new(resource: ResourceBox) -> Self {
        Self {
            resource,
            compiled_data: RefCell::new(Vec::new()),
        }
    }
}

pub struct CompiledResources<'a> {
    pub options: CompilerOptions,
    data: &'a HashMap<ResourceId, GraphNode>,
}

impl<'a> CompiledResources<'a> {
    pub fn get_resource_data(&self, id: ResourceId) -> Ref<'_, Vec<u8>> {
        self.data.get(&id).unwrap().compiled_data.borrow()
    }
}

fn add_resource_dependacy(
    context: &mut CompilerGraph,
    resources: &mut HashMap<ResourceId, GraphNode>,
    mut dependencies: Vec<(ResourceId, Option<ResourceBox>)>,
    parent_id: ResourceId,
) {
    for (id, _) in dependencies.iter() {
        context.graph.add_parent(node_index(parent_id.0), (), id.0);
    }
    resources.extend(
        dependencies
            .drain(..)
            .filter_map(|(id, resource)| resource.map(|r| (id, r)))
            .map(|(id, resource)| (id, GraphNode::new(resource))),
    );
}

pub fn compile(resource: ResourceBox, options: CompilerOptions) -> Vec<u8> {
    let mut context = CompilerGraph {
        options: options.clone(),
        graph: daggy::Dag::<usize, ()>::new(),
        current_resource_id: Cell::new(0),
    };
    let mut resources = HashMap::new();

    let root_index = context.get_next_resource_id();
    resources.insert(root_index, GraphNode::new(resource));

    let root = context.graph.add_node(root_index.0);

    let dependencies = {
        resources
            .get_mut(&root_index)
            .unwrap()
            .resource
            .extract_dependencies(&context)
    };
    add_resource_dependacy(&mut context, &mut resources, dependencies, root_index);

    let mut nodes_to_visit = vec![];
    // Recursive build of graph
    // We are using parents function because we want to have reverse dag, dependancies to point to parent
    let mut child_walker = context.graph.parents(root);
    loop {
        match child_walker.walk_next(&context.graph) {
            Some((_, node)) => {
                nodes_to_visit.push(node);
                let dependencies = {
                    resources
                        .get_mut(&ResourceId(node.index()))
                        .unwrap()
                        .resource
                        .extract_dependencies(&context)
                };
                add_resource_dependacy(
                    &mut context,
                    &mut resources,
                    dependencies,
                    ResourceId(node.index()),
                );
            }
            None => {
                if nodes_to_visit.is_empty() {
                    break;
                }
                let next_node = nodes_to_visit.remove(0);
                child_walker = context.graph.parents(next_node);
            }
        }
    }

    // Topological sort
    let sorted_nodes = petgraph::algo::toposort(context.graph.graph(), None)
        .expect("Compiler graph is not a DAG!");

    // Start compilation phase
    let compiled_dependencies = CompiledResources { options, data: &resources };

    for node in sorted_nodes {
        let compiled_data = resources
            .get(&ResourceId(node.index()))
            .unwrap()
            .resource
            .compile(&compiled_dependencies);
        resources
            .get(&ResourceId(node.index()))
            .unwrap()
            .compiled_data
            .replace(compiled_data);
    }

    let result = resources
        .get(&root_index)
        .unwrap()
        .compiled_data
        .replace(vec![]);
    result
}
