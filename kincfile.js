let project = new Project('TriangleVR');

project.addFile('Sources/**');
project.setDebugDir('Deployment');

resolve(project);
