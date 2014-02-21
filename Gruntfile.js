module.exports = function (grunt) {
	var matchdep = require('matchdep'),
	    commonTasks

	matchdep.filter('grunt-*').forEach(grunt.loadNpmTasks)

	grunt.initConfig({
		pkg: grunt.file.readJSON('package.json'),

		clean: {
			all: [
				'webroot/static',
				'webroot/views',
			],
		},
		compress: {
			all: {
				options: {
					mode: 'gzip',
				},
				files: [{
					expand: true,
					cwd: 'webroot',
					src: [
						'static/js/**/*.js',
						'static/css/**/*.css',
						'views/**/*.html',
					],
					dest: 'webroot',
				}],
			},
		},
		cssmin: {
			all: {
				files: [{
					expand: true,
					cwd: 'webroot/static/css',
					src: '**/*.css',
					dest: 'webroot/static/css',
				}],
			},
		},
		htmlmin: {
			all: {
				options: {
					collapseWhitespace: true,
					collapseBooleanAttributes: true,
					removeAttributeQuotes: true,
					removeRedundantAttributes: true,
					removeEmptyAttributes: true,
					removeOptionalTags: true,
				},
				files: [{
					expand: true,
					cwd: 'webroot/',
					src: '**/*.html',
					dest: 'webroot/',
				}],
			},
		},
		imagemin: {
			all: {
				options: {
					optimizationLevel: 3,
					progressive: true,
				},
				files: [{
					expand: true,
					cwd: 'webroot/static/img',
					src: '**/*.{png,jpg}',
					dest: 'webroot/static/img',
				}],
			},
		},
		jade: {
			development: {
				options: {
					pretty: true,
					data: {
						__min: '',
						__debug: true,
					},
				},
				files: [{
					expand: true,
					cwd: 'app/views',
					src: '**/*.jade',
					dest: 'webroot/views/',
					ext: '.html',
				}],
			},
			production: {
				options: {
					pretty: false,
					data: {
						__min: '.min',
						__debug: false,
					},
				},
				files: [{
					expand: true,
					cwd: 'app/views',
					src: '**/*.jade',
					dest: 'webroot/',
					ext: '.html',
				}],
			},
		},
		stylus: {
			development: {
				options: {
					debug: true,
					compress: false,
					'include css': true,
					use: [
						require('nib'),
					],
					'import': [
						'nib',
					],
				},
				files: [{
					'webroot/static/css/main.css': [
						'app/assets/styl/main.styl',
					],
				}],
			},
			production: {
				options: {
					debug: false,
					compress: true,
					'include css': true,
					use: [
						require('nib'),
					],
					'import': [
						'nib',
					],
				},
				files: [{
					'webroot/static/css/main.css': [
						'app/assets/styl/main.styl',
					],
				}],
			},
		},
		watch: {
			options: {
				spawn: false,
				atBegin: true,
			},
			stylus: {
				files: ['app/assets/styl/**/*.styl'],
				tasks: ['stylus:production'],
			},
			jade: {
				files: ['app/views/**/*.jade'],
				tasks: ['jade:production'],
			},
			livereload: {
				files: ['webroot/static/css/**/*.css'],
				tasks: [],
				options: {
					livereload: true,
				},
			},
		},
		concurrent: {
			options: {
				logConcurrentOutput: true,
			},
			all: [
				'watch',
			],
		},
		sync: {
			all: {
				files: [{
					cwd: 'app/assets',
					src: '{js,font,img,etc}/**',
					dest: 'webroot/static/',
				}],
			},
		},
	})

	commonTasks = [
		'clean',
		'sync',
	]

	grunt.registerTask('production', commonTasks.concat([
		'stylus',
		'jade',

		// production only:
		'cssmin',
		'imagemin',
		'htmlmin',
		'compress',
	]))
	grunt.registerTask('development', commonTasks.concat([
		'concurrent',
	]))
}
