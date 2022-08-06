#!/usr/bin/env groovy

pipeline {

  agent any

  stages {
    stage('Prepare'){
      steps {
        sh 'echo 1'
      }
    }

    stage('Install'){
      steps {
        sh 'echo 2'
      }
    }

    stage('Test'){
      steps {
        sh 'echo 3'
      }
    }

    stage('Clean up'){
      steps {
        sh 'echo 4'
      }
    }
  }
}
