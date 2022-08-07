#!/usr/bin/env groovy


def python_version = params.getOrDefault("python_version", "3.9")
def python_image = "python:${python_version}"
def python_container_cmd = '-u root -v ${WORKSPACE}@tmp:/workspace -v ${WORKSPACE}:/sources'

def telegram_url = "https://api.telegram.org/bot${BOT_TOKEN}/sendMessage"


pipeline {

  agent any

  parameters {
    string(
      name: 'python_version',
      defaultValue: '3.9'
    )
  }

  stages {
    stage('Prepare'){
      steps {
        script {
            docker.image(python_image).inside(python_container_cmd){
                sh "python -m venv /workspace/venv"
            }
        }
      }
    }

    stage('Install'){
      steps {
        script {
            docker.image(python_image).inside(python_container_cmd){
                sh "cd /sources && /workspace/venv/bin/pip install -r requirements.txt"
                sh "cd /sources && /workspace/venv/bin/pip install -v '.[all]'"
            }
        }
      }
    }

    stage('Test'){
      steps {
        script {
            docker.image(python_image).inside(python_container_cmd){
                sh "cd /sources && /workspace/venv/bin/python ./setup.py test"
            }
        }
      }
    }
  }  // stages

  post {

    fixed { 
      withCredentials([
        string(credentialsId: 'telegramBotToken', variable: 'BOT_TOKEN'),
        string(credentialsId: 'telegramChatId', variable: 'CHAT_ID')
      ]) {
        
        message = "The job <b>'${env.JOB_NAME}'</b> fixed: branch '${env.GIT_BRANCH}', build number ${env.BUILD_NUMBER}. Details: ${env.BUILD_URL}"
        
        sh """
          curl -X POST ${telegram_url} -d chat_id=${CHAT_ID} -d parse_mode=HTML -d text='${message}'
        """
      }
    }
        
    aborted {
      withCredentials([
        string(credentialsId: 'telegramBotToken', variable: 'BOT_TOKEN'),
        string(credentialsId: 'telegramChatId', variable: 'CHAT_ID')
      ]) {
        
        message = "The job <b>'${env.JOB_NAME}'</b> aborted: branch '${env.GIT_BRANCH}', build number ${env.BUILD_NUMBER}. Details: ${env.BUILD_URL}"
        
        sh """
          curl -X POST ${telegram_url} -d chat_id=${CHAT_ID} -d parse_mode=HTML -d text='${message}'
        """
      }        
    }
    
    failure {
      withCredentials([
        string(credentialsId: 'telegramBotToken', variable: 'BOT_TOKEN'),
        string(credentialsId: 'telegramChatId', variable: 'CHAT_ID')
      ]) {
        
        message = "The job <b>'${env.JOB_NAME}'</b> failed: branch '${env.GIT_BRANCH}', build number ${env.BUILD_NUMBER}. Details: ${env.BUILD_URL}"
        
        sh """
          curl -X POST ${telegram_url} -d chat_id=${CHAT_ID} -d parse_mode=HTML -d text='${message}'
        """
      }        
    }
    
    success {
      withCredentials([
        string(credentialsId: 'telegramBotToken', variable: 'BOT_TOKEN'),
        string(credentialsId: 'telegramChatId', variable: 'CHAT_ID')
      ]) {
        
        message = "The job <b>'${env.JOB_NAME}'</b> completed successfully: branch '${env.GIT_BRANCH}', build number ${env.BUILD_NUMBER}. Details: ${env.BUILD_URL}"
        
        sh """
          curl -X POST ${telegram_url} -d chat_id=${CHAT_ID} -d parse_mode=HTML -d text='${message}'
        """
      }
    }    
        
  }
    
}
