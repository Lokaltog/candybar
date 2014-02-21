// This will be created inside the WklineCtrl scope, but it will be globally
// available for data injection into the controller scope
var wkInject

angular.module('Wkline', [])
	.controller('WklineCtrl', ['$scope', function ($scope) {
		$scope.data = {}
		wkInject = function (payload) {
			console.log('Data received!')
			$scope.$apply(function () {
				$scope.data = payload
			})
		}
	}])
